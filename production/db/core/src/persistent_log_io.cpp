/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "persistent_log_io.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>

#include <atomic>
#include <memory>
#include <set>
#include <string>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/write_ahead_log_error.hpp"
#include "gaia_internal/db/db_types.hpp"
#include <gaia_internal/common/mmap_helpers.hpp>

#include "crc32c.h"
#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "liburing.h"
#include "memory_types.hpp"
#include "wal_file.hpp"

using namespace gaia::common;
using namespace gaia::db;

namespace gaia
{
namespace db
{

wal_writer_t::wal_writer_t(std::string& dir_path_, int dir_fd_)
{
    dir_path = dir_path_;
    dir_fd = dir_fd_;

    // Create new wal file every time the wal writer gets initialized.
    current_file.reset(new wal_file_t(dir_path, dir_fd, file_num, file_size));
    io_uring_manager = std::make_unique<io_uring_manager_t>();
    io_uring_manager->open();
}

wal_writer_t::~wal_writer_t()
{
    close_fd(dir_fd);
}

// Currently defaulting to the rocks/leveldb impl.
// Todo(Mihir) - Research more crc libs / write cmake for rocks util folder so it can be linked on its own.
uint32_t calculate_crc32(uint32_t init_crc, void* data, size_t n)
{
    // This implementation uses the CRC32 instruction from the SSE4 (SSE4.2) instruction set if it is available.
    // From my understanding of the code, it defaults to a 4 table based lookup implementation otherwise.
    // Here is an old benchmark that compares various crc impl including the two used by rocks. https://www.strchr.com/crc32_popcnt
    return rocksdb::crc32c::Extend(init_crc, (const char*)data, n);
}

void wal_writer_t::process_txn_log(txn_log_t* txn_log, gaia_txn_id_t commit_ts)
{
    // Sort offsets.
    std::set<gaia_offset_t> object_offsets;
    for (size_t i = 0; i < txn_log->record_count; i++)
    {
        auto lr = txn_log->log_records + i;
        object_offsets.insert(lr->new_offset);
    }

    // Group contiguous objects and also find total object size.
    // We expect that objects are assigned contiguously in a txn within the same memory chunk. Add asserts accordingly.
    size_t payload_size = 0;
    std::vector<gaia_offset_t> contiguous_offsets;
    gaia_offset_t current_chunk_offset = c_invalid_gaia_offset;

    for (auto it = object_offsets.begin(); it != object_offsets.end(); it++)
    {
        auto offset = *it;

        // First entry.
        if (current_chunk_offset == c_invalid_gaia_offset)
        {
            contiguous_offsets.push_back(offset);

            // Set chunk offset for the first entry.
            current_chunk_offset = offset / gaia::db::memory_manager::c_chunk_size;
            continue;
        }

        // Last entry.
        if (it == object_offsets.end())
        {
            auto end_offset = offset + offset_to_ptr(offset)->payload_size;

            ASSERT_INVARIANT(current_chunk_offset == end_offset % gaia::db::memory_manager::c_chunk_size, "Object cannot be split across chunks");

            payload_size += end_offset - contiguous_offsets.at(contiguous_offsets.size() - 1);

            contiguous_offsets.push_back(end_offset);
            continue;
        }

        if (current_chunk_offset != offset / gaia::db::memory_manager::c_chunk_size)
        {
            auto end_offset = current_chunk_offset + gaia::db::memory_manager::c_chunk_size;

            payload_size += end_offset - contiguous_offsets.at(contiguous_offsets.size() - 1);

            // Insert end of chunk offset.
            contiguous_offsets.push_back(end_offset);

            // Insert offset of the new object.
            contiguous_offsets.push_back(offset);

            // Update current chunk offset.
            current_chunk_offset = offset / gaia::db::memory_manager::c_chunk_size;
        }
    }

    ASSERT_INVARIANT(contiguous_offsets.size() % 2 == 0, "We expect a begin and end offset.");

    // For simplicity, we don't break up transaction records across log files. We simply
    // write it to the next log file. If a transaction is greater in size than size of the log file,
    // then we write out the entire txn in the new log file. In this case log files.
    // TODO: ASSERT(payload_size < log_file_size)
    if (!current_file->has_enough_space(payload_size))
    {
        io_uring_manager->handle_file_close(current_file->file_fd);
        current_file.reset();

        // Open new file.
        file_num++;
        auto fs = (payload_size > file_size) ? payload_size : file_size;
        current_file = std::make_unique<wal_file_t>(dir_path, dir_fd, file_num, fs);
    }

    // Augment payload size with the size of deleted ids.
    payload_size += txn_log->deleted_count * sizeof(gaia_id_t);

    // Finally make call.
    create_txn_record(commit_ts, payload_size, record_type_t::txn, contiguous_offsets, &txn_log->deleted_ids[0], txn_log->deleted_count);
}

void wal_writer_t::create_decision_record(
    std::vector<std::pair<gaia_txn_id_t, txn_decision_type_t>>& txn_decisions)
{
}

// Test: Encode multiple txn records.
// Max number of SQEs. You need to reduce the number of pwritecalls.
// Per txn batch maintain a helper buffer with size
// This API assumes that we preprocessed the offsets and only send object
// offsets that can fit into the current log file.
// Assumption is all writes fit inside current log file.
void wal_writer_t::create_txn_record(
    gaia_txn_id_t commit_ts,
    size_t payload_size, // Excludes the header.
    record_type_t type,
    std::vector<gaia_offset_t>& object_offsets,
    gaia_id_t* deleted_ids,
    size_t deleted_id_count)
{
    std::vector<iovec> writes_to_submit;

    record_header_t header;
    header.crc = c_crc_initial_value;
    header.payload_size = payload_size;
    header.deleted_count = deleted_id_count;
    header.txn_commit_ts = commit_ts;
    header.record_type = type;
    header.entry.index = current_file->file_num;
    header.entry.offset = current_file->current_offset;

    // Compute CRC and extend it with the objects.
    crc32_t txn_crc = 0;
    txn_crc = calculate_crc32(txn_crc, &header, sizeof(record_header_t));

    // Reserve iovec to store header for the log record.
    struct iovec header_entry = {nullptr, 0};
    writes_to_submit.push_back(header_entry);

    // Calculate crc and create pwrite entries.
    for (size_t i = 0; i < object_offsets.size(); i += 2)
    {
        auto ptr = offset_to_ptr(object_offsets.at(i));
        auto chunk_size = object_offsets.at(i + 1) - object_offsets.at(i);

        txn_crc = calculate_crc32(txn_crc, ptr, chunk_size);

        struct iovec temp;
        temp.iov_base = ptr;
        temp.iov_len = chunk_size;
        writes_to_submit.push_back(temp);
    }

    auto deleted_size = deleted_id_count * sizeof(gaia_id_t);

    txn_crc = calculate_crc32(txn_crc, deleted_ids, deleted_size);

    // Update CRC before writing it.
    header.crc = txn_crc;

    auto header_ptr = io_uring_manager->copy_into_helper_buffer(&header, sizeof(record_header_t), current_file->file_fd);

    // Update the first iovec entry with the header information.
    writes_to_submit.at(0).iov_base = header_ptr;
    writes_to_submit.at(0).iov_len = sizeof(record_header_t);

    // Allocate space for deleted writes in helper buffer.
    auto deleted_id_ptr = io_uring_manager->copy_into_helper_buffer(deleted_ids, deleted_size, current_file->file_fd);
    writes_to_submit.push_back({deleted_id_ptr, deleted_size});

    io_uring_manager->construct_pwritev(writes_to_submit, current_file->file_fd, current_file->current_offset);
}

size_t
wal_reader_t::get_next_wal_record(struct record_iterator* it, read_record_t* destination)
{
    if (it->cursor < it->stop_at)
    {
        size_t record_size = update_cursor(it, destination);

        // No more records in file.
        if (record_size == 0)
        {
            it->cursor = NULL;
            it->end = NULL;
            return -1;
        }

        recover_gaia_objects_from_txn_record(destination, record_size);

        return record_size;
    }

    it->cursor = NULL;
    it->end = NULL;
    return -1;
}

// This API could be made generic based on type of the destination pointer.
size_t
wal_reader_t::update_cursor(struct record_iterator* it, read_record_t* destination)
{
    destination = reinterpret_cast<read_record_t*>(it->cursor);
    if (destination->header.crc == 0)
    {
        is_remaining_file_empty(it->cursor, it->stop_at);
        return 0;
    }
    else
    {
        // We never expect CRC of a record to be empty if the rest of the file isn't also empty.
        throw write_ahead_log_error("Read log record with empty checksum value.");
    }

    auto expected_crc = destination->header.crc;
    destination->header.crc = c_crc_initial_value;

    // Make sure record checksum is correct.
    auto record_size = sizeof(struct record_header_t) + destination->header.payload_size;
    auto crc = calculate_crc32(0, destination, record_size);

    if (crc != expected_crc)
    {
        throw new write_ahead_log_error("Record checksum match failed!");
    }

    // Update cursor.
    it->cursor = it->cursor + record_size;

    return record_size;
}

void wal_reader_t::map_log_file(struct record_iterator* it, int file_fd, log_offset_t start_offset)
{
    struct stat st;
    read_record_t* wal_record;

    if (fstat(file_fd, &st) == -1)
    {
        throw new write_ahead_log_error("failed to fstat wal file", errno);
    }

    auto first_data = lseek(file_fd, 0, SEEK_DATA);
    ASSERT_INVARIANT(first_data == 0, "We don't expect any holes in the beginning to the wal files.");
    ASSERT_INVARIANT(st.st_size > 0, "We don't expect to write empty persistent log files.");

    gaia::common::map_fd_data(
        wal_record,
        st.st_size,
        PROT_READ,
        MAP_SHARED,
        file_fd,
        start_offset);

    *it = (struct record_iterator){
        .cursor = (uint8_t*)wal_record,
        .end = (uint8_t*)wal_record + st.st_size,
        .stop_at = (uint8_t*)wal_record + st.st_size,
        .begin = (uint8_t*)wal_record,
        .mapped = wal_record,
        .map_size = (size_t)st.st_size,
        .file_fd = file_fd};
}

bool wal_reader_t::is_remaining_file_empty(uint8_t* start, uint8_t* end)
{
    auto remaining_size = end - start;
    uint8_t zeroblock[remaining_size];
    memset(zeroblock, 0, sizeof zeroblock);
    return memcmp(zeroblock, start, remaining_size) == 0;
}

void wal_reader_t::unmap_file(struct record_iterator* it)
{
    munmap(it->mapped, it->map_size);
}

} // namespace db
} // namespace gaia
