/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "log_io.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>

#include <atomic>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <string>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/common/write_ahead_log_error.hpp"
#include "gaia_internal/db/db_types.hpp"
#include <gaia_internal/common/mmap_helpers.hpp>

#include "crc32c.h"
#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_object_helpers.hpp"
#include "liburing.h"
#include "log_file.hpp"
#include "mapped_data.hpp"
#include "memory_manager.hpp"
#include "memory_types.hpp"
#include "txn_metadata.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;
using namespace gaia::db;

namespace gaia
{
namespace db
{
namespace persistence
{

log_handler_t::log_handler_t(const std::string& directory_path)
{
    auto dirpath = directory_path;
    ASSERT_PRECONDITION(!dirpath.empty(), "Gaia persistent directory path shouldn't be empty.");
    s_wal_dir_path = dirpath.append(c_gaia_wal_dir_name);
    auto code = mkdir(s_wal_dir_path.c_str(), c_gaia_wal_dir_permissions);
    if (code == -1 && errno != EEXIST)
    {
        throw_system_error("Unable to create persistent log directory");
    }
    dir_fd = open(s_wal_dir_path.c_str(), O_DIRECTORY);
    if (dir_fd <= 0)
    {
        throw_system_error("Unable to open persistent log directory.");
    }
}

void log_handler_t::open_for_writes(int validate_flushed_batch_efd, int signal_checkpoint_eventfd)
{
    ASSERT_PRECONDITION(validate_flushed_batch_efd >= 0, "Invalid validate flush eventfd.");
    ASSERT_PRECONDITION(signal_checkpoint_eventfd >= 0, "Invalid checkpoint eventfd.");
    ASSERT_INVARIANT(dir_fd > 0, "Unable to open data directory for persistent log writes.");

    // Create new wal file every time the wal writer gets initialized.
    async_disk_writer = std::make_unique<async_disk_writer_t>(validate_flushed_batch_efd, signal_checkpoint_eventfd);

    async_disk_writer->open();
}

log_handler_t::~log_handler_t()
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
    return rocksdb::crc32c::Extend(init_crc, static_cast<const char*>(data), n);
}

file_offset_t log_handler_t::allocate_log_space(size_t payload_size)
{
    // For simplicity, we don't break up transaction records across log files. We simply
    // write it to the next log file. If a transaction is greater in size than size of the log file,
    // then we write out the entire txn in the new log file. In this case log files.
    // TODO: ASSERT(payload_size < log_file_size)
    if (!current_file)
    {
        auto fs = (payload_size > c_file_size) ? payload_size : c_file_size;
        current_file.reset();
        current_file = std::make_unique<log_file_t>(s_wal_dir_path, dir_fd, file_num, fs);
    }
    else if (current_file->get_remaining_bytes_count(payload_size) <= 0)
    {
        async_disk_writer->perform_file_close_operations(current_file->get_file_fd(), current_file->get_file_sequence());

        // As a simplification, one batch writes to a single log file at a time.
        async_disk_writer->submit_and_swap_in_progress_batch(current_file->get_file_fd());

        current_file.reset();

        // Open new file.
        file_num++;
        auto fs = (payload_size > c_file_size) ? payload_size : c_file_size;
        current_file = std::make_unique<log_file_t>(s_wal_dir_path, dir_fd, file_num, fs);
    }

    auto current_offset = current_file->get_current_offset();
    current_file->allocate(payload_size);
    return current_offset;
}

void log_handler_t::create_decision_record(decision_list_t& txn_decisions)
{
    ASSERT_PRECONDITION(!txn_decisions.empty(), "Decision record cannot have empty payload.");
    // Track decisions per batch.
    async_disk_writer->add_decisions_to_batch(txn_decisions);

    // Create decision record and write it using pwritev.
    std::vector<iovec> writes_to_submit;
    size_t txn_decision_size = txn_decisions.size() * (sizeof(gaia_txn_id_t) + sizeof(decision_type_t));
    auto total_log_space_needed = txn_decision_size + sizeof(record_header_t);
    allocate_log_space(total_log_space_needed);

    record_header_t header;
    header.crc = c_crc_initial_value;
    header.payload_size = total_log_space_needed;
    header.count = txn_decisions.size();
    header.txn_commit_ts = 0;
    header.record_type = record_type_t::decision;

    crc32_t txn_crc = 0;
    txn_crc = calculate_crc32(txn_crc, &header, sizeof(record_header_t));
    txn_crc = calculate_crc32(txn_crc, txn_decisions.data(), txn_decision_size);

    ASSERT_INVARIANT(txn_crc > 0, "CRC cannot be zero.");
    header.crc = txn_crc;

    auto header_ptr = async_disk_writer->copy_into_metadata_buffer(&header, sizeof(record_header_t), current_file->get_file_fd());
    auto txn_decisions_ptr = async_disk_writer->copy_into_metadata_buffer(txn_decisions.data(), txn_decision_size, current_file->get_file_fd());

    writes_to_submit.push_back({header_ptr, sizeof(record_header_t)});
    writes_to_submit.push_back({txn_decisions_ptr, txn_decision_size});

    async_disk_writer->enqueue_pwritev_requests(writes_to_submit, current_file->get_file_fd(), current_file->get_current_offset(), uring_op_t::pwritev_decision);
}

void log_handler_t::process_txn_log_and_write(int txn_log_fd, gaia_txn_id_t commit_ts, memory_manager_t* memory_manager)
{
    mapped_log_t log;
    log.open(txn_log_fd);

    map_commit_ts_to_session_unblock_fd(commit_ts, log.data()->session_decision_fd);

    std::vector<common::gaia_id_t> deleted_ids;
    std::map<chunk_offset_t, std::set<gaia_offset_t>> map;

    // Init map.
    for (size_t i = 0; i < log.data()->chunk_count; i++)
    {
        auto chunk = log.data()->chunks + i;
        auto chunk_offset = memory_manager->get_chunk_offset(get_address_offset(*chunk));
        map.insert(std::pair(chunk_offset, std::set<gaia_offset_t>()));
    }

    // Obtain deleted_ids & obtain sorted offsets per chunk.
    for (size_t i = 0; i < log.data()->record_count; i++)
    {
        auto lr = log.data()->log_records + i;
        if (lr->operation == gaia_operation_t::remove)
        {
            deleted_ids.push_back(lr->deleted_id);
        }
        else
        {
            auto chunk = memory_manager->get_chunk_offset(get_address_offset(lr->new_offset));
            ASSERT_INVARIANT(map.count(chunk) > 0, "Can't find chunk.");
            ASSERT_INVARIANT(chunk != c_invalid_chunk_offset, "Invalid chunk offset found.");
            map.find(chunk)->second.insert(get_address_offset(lr->new_offset));
        }
    }

    // Group contiguous objects and also find total object size.
    // We expect that objects are assigned contiguously in a txn within the same memory chunk. Add asserts accordingly.
    std::vector<gaia_offset_t> contiguous_offsets;

    for (const auto& pair : map)
    {
        auto object_address_offsets = pair.second;
        if (object_address_offsets.size() == 0)
        {
            continue;
        }
        contiguous_offsets.push_back(*object_address_offsets.begin());
        auto it = object_address_offsets.end();
        it--;
        auto end_offset = *it;
        auto payload_size = offset_to_ptr(get_gaia_offset(end_offset))->payload_size + c_db_object_header_size;
        size_t allocation_size = base_memory_manager_t::calculate_allocation_size(payload_size);
        contiguous_offsets.push_back(end_offset + allocation_size);
    }

    ASSERT_INVARIANT(contiguous_offsets.size() % 2 == 0, "We expect a begin and end offset.");

    if (deleted_ids.size() > 0 || contiguous_offsets.size() > 0)
    {
        // Finally make call.
        create_txn_record(commit_ts, record_type_t::txn, contiguous_offsets, deleted_ids);
    }
}

void log_handler_t::map_commit_ts_to_session_unblock_fd(gaia_txn_id_t commit_ts, int session_unblock_fd)
{
    ASSERT_INVARIANT(session_unblock_fd > 0, "incorrect session unblock fd");
    async_disk_writer->map_commit_ts_to_session_decision_efd(commit_ts, session_unblock_fd);
}

void log_handler_t::validate_flushed_batch()
{
    async_disk_writer->perform_post_completion_maintenance();
}

void log_handler_t::submit_writes(bool sync)
{
    async_disk_writer->submit_and_swap_in_progress_batch(current_file->get_file_fd(), sync);
}

void log_handler_t::create_txn_record(
    gaia_txn_id_t commit_ts,
    record_type_t type,
    std::vector<gaia_offset_t>& contiguous_address_offsets,
    std::vector<gaia_id_t>& deleted_ids)
{
    ASSERT_PRECONDITION(!deleted_ids.empty() || (contiguous_address_offsets.size() % 2 == 0 && !contiguous_address_offsets.empty()), "Txn record cannot have empty payload.");
    std::vector<iovec> writes_to_submit;

    // Reserve iovec to store header for the log record.
    struct iovec header_entry = {nullptr, 0};
    writes_to_submit.push_back(header_entry);

    // Create iovec entries.
    size_t payload_size = 0;
    for (size_t i = 0; i < contiguous_address_offsets.size(); i += 2)
    {
        auto offset = get_gaia_offset(contiguous_address_offsets.at(i));
        auto ptr = offset_to_ptr(offset);
        auto chunk_size = contiguous_address_offsets.at(i + 1) - contiguous_address_offsets.at(i);
        payload_size += chunk_size;
        writes_to_submit.push_back({ptr, chunk_size});
    }

    // Augment payload size with the size of deleted ids.
    auto deleted_size = deleted_ids.size() * sizeof(gaia_id_t);
    payload_size += deleted_size;

    // Calculate total log space needed.
    auto total_log_space_needed = payload_size + sizeof(record_header_t);

    // Allocate log space.
    allocate_log_space(total_log_space_needed);

    // Create header.
    record_header_t header;
    header.crc = c_crc_initial_value;
    header.payload_size = total_log_space_needed;
    header.count = deleted_ids.size();
    header.txn_commit_ts = commit_ts;
    header.record_type = type;

    // Calculate CRC.
    auto txn_crc = calculate_crc32(0, &header, sizeof(record_header_t));

    // Start from 1 to skip the first entry.
    for (size_t i = 1; i < writes_to_submit.size(); i++)
    {
        txn_crc = calculate_crc32(txn_crc, writes_to_submit.at(i).iov_base, writes_to_submit.at(i).iov_len);
    }

    // Delete CRC at the end.
    txn_crc = calculate_crc32(txn_crc, deleted_ids.data(), deleted_size);

    // Update CRC in header before writing it.
    ASSERT_INVARIANT(txn_crc > 0, "CRC cannot be zero.");
    header.crc = txn_crc;

    auto header_ptr = async_disk_writer->copy_into_metadata_buffer(&header, sizeof(record_header_t), current_file->get_file_fd());

    // Update the first iovec entry with the header information.
    writes_to_submit.at(0).iov_base = header_ptr;
    writes_to_submit.at(0).iov_len = sizeof(record_header_t);

    // Allocate space for deleted writes in helper buffer.
    if (!deleted_ids.empty())
    {
        auto deleted_id_ptr = async_disk_writer->copy_into_metadata_buffer(deleted_ids.data(), deleted_size, current_file->get_file_fd());
        writes_to_submit.push_back({deleted_id_ptr, deleted_size});
    }

    async_disk_writer->enqueue_pwritev_requests(writes_to_submit, current_file->get_file_fd(), current_file->get_current_offset(), uring_op_t::pwritev_txn);
}

} // namespace persistence
} // namespace db
} // namespace gaia
