////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "log_io.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>

#include <atomic>
#include <filesystem>
#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <string>

#include <sys/stat.h>
#include "liburing.h"

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/db/db_types.hpp"
#include <gaia_internal/common/mmap_helpers.hpp>

#include "crc32c.h"
#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_object_helpers.hpp"
#include "log_file.hpp"
#include "mapped_data.hpp"
#include "memory_helpers.hpp"
#include "memory_types.hpp"
#include "persistence_types.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;
using namespace gaia::db;

namespace gaia
{
namespace db
{
namespace persistence
{

log_handler_t::log_handler_t(const std::string& data_dir_path)
{
    ASSERT_PRECONDITION(!data_dir_path.empty(), "Gaia persistent data directory path shouldn't be empty.");

    std::filesystem::path wal_dir_parent = data_dir_path;
    std::filesystem::path wal_dir_child = c_gaia_wal_dir_name;
    s_wal_dir_path = wal_dir_parent / wal_dir_child;

    if (-1 == ::mkdir(s_wal_dir_path.c_str(), c_gaia_wal_dir_permissions) && errno != EEXIST)
    {
        throw_system_error("Unable to create persistent log directory");
    }

    if (-1 == ::open(s_wal_dir_path.c_str(), O_DIRECTORY))
    {
        throw_system_error("Unable to open persistent log directory.");
    }
}

void log_handler_t::open_for_writes(int validate_flushed_batch_eventfd, int signal_checkpoint_eventfd)
{
    ASSERT_PRECONDITION(validate_flushed_batch_eventfd != -1, "Eventfd to signal post-flush maintenance operations invalid!");
    ASSERT_PRECONDITION(signal_checkpoint_eventfd != -1, "Eventfd to signal checkpointing of log file is invalid!");
    ASSERT_INVARIANT(s_dir_fd != -1, "Unable to open data directory for persistent log writes.");

    // Create new log file every time the log_writer gets initialized.
    m_async_disk_writer = std::make_unique<async_disk_writer_t>(validate_flushed_batch_eventfd, signal_checkpoint_eventfd);

    m_async_disk_writer->open();
}

log_handler_t::~log_handler_t()
{
    close_fd(s_dir_fd);
}

// Currently using the rocksdb impl.
// Todo(Mihir) - Research other crc libs.
uint32_t calculate_crc32(uint32_t init_crc, const void* data, size_t n)
{
    // This implementation uses the CRC32 instruction from the SSE4 (SSE4.2) instruction set if it is available.
    // Otherwise, it defaults to a 4 table based lookup implementation.
    // Here is an old benchmark that compares various crc implementations including the two used by rocks.
    // https://www.strchr.com/crc32_popcnt
    return rocksdb::crc32c::Extend(init_crc, static_cast<const char*>(data), n);
}

file_offset_t log_handler_t::allocate_log_space(size_t payload_size)
{
    // For simplicity, we don't break up transaction records across log files. Txn updates
    // which don't fit in the current file are written to the next one.
    // If a transaction is larger than the log file size, then the entire txn is written to the next log file.
    // Another simplification is that an async_write_batch contains only writes belonging to a single log file.
    if (!m_current_file)
    {
        auto file_size = (payload_size > c_max_log_file_size_in_bytes) ? payload_size : c_max_log_file_size_in_bytes;
        m_current_file.reset();
        m_current_file = std::make_unique<log_file_t>(s_wal_dir_path, s_dir_fd, file_sequence_t(s_file_num), file_size);
    }
    else if (m_current_file->get_bytes_remaining_after_append(payload_size) <= 0)
    {
        m_async_disk_writer->perform_file_close_operations(
            m_current_file->get_file_fd(), m_current_file->get_file_sequence());

        // One batch writes to a single log file at a time.
        m_async_disk_writer->submit_and_swap_in_progress_batch(m_current_file->get_file_fd());

        m_current_file.reset();

        // Open new file.
        s_file_num++;
        auto file_size = (payload_size > c_max_log_file_size_in_bytes) ? payload_size : c_max_log_file_size_in_bytes;
        m_current_file = std::make_unique<log_file_t>(s_wal_dir_path, s_dir_fd, file_sequence_t(s_file_num), file_size);
    }

    auto current_offset = m_current_file->get_current_offset();
    m_current_file->allocate(payload_size);

    // Return starting offset of the allocation.
    return current_offset;
}

void log_handler_t::create_decision_record(const decision_list_t& txn_decisions)
{
    ASSERT_PRECONDITION(!txn_decisions.empty(), "Decision record cannot have empty payload.");

    // Track decisions per batch.
    m_async_disk_writer->add_decisions_to_batch(txn_decisions);

    // Create decision record and enqueue a pwrite() request for the same.
    std::vector<iovec> writes_to_submit;
    size_t txn_decision_size = txn_decisions.size() * (sizeof(decision_entry_t));
    size_t total_log_space_needed = txn_decision_size + sizeof(record_header_t);
    allocate_log_space(total_log_space_needed);

    // Create log record header.
    record_header_t header{};
    header.crc = c_crc_initial_value;
    header.payload_size = total_log_space_needed;
    header.decision_count = txn_decisions.size();
    header.txn_commit_ts = c_invalid_gaia_txn_id;
    header.record_type = record_type_t::decision;

    // Compute crc.
    crc32_t txn_crc = 0;
    txn_crc = calculate_crc32(txn_crc, &header, sizeof(record_header_t));
    txn_crc = calculate_crc32(txn_crc, txn_decisions.data(), txn_decision_size);

    ASSERT_INVARIANT(txn_crc != 0, "CRC cannot be zero.");
    header.crc = txn_crc;

    // Copy information which needs to survive for the entire batch lifetime into the metadata buffer.
    auto header_ptr = m_async_disk_writer->copy_into_metadata_buffer(
        &header, sizeof(record_header_t), m_current_file->get_file_fd());
    auto txn_decisions_ptr = m_async_disk_writer->copy_into_metadata_buffer(
        txn_decisions.data(), txn_decision_size, m_current_file->get_file_fd());

    writes_to_submit.push_back({header_ptr, sizeof(record_header_t)});
    writes_to_submit.push_back({txn_decisions_ptr, txn_decision_size});

    m_async_disk_writer->enqueue_pwritev_requests(
        writes_to_submit, m_current_file->get_file_fd(),
        m_current_file->get_current_offset(), uring_op_t::pwritev_decision);
}

void log_handler_t::process_txn_log_and_write(log_offset_t log_offset, gaia_txn_id_t commit_ts)
{
    // Map in memory txn_log.
    auto txn_log = get_txn_log_from_offset(log_offset);

    std::vector<common::gaia_id_t> deleted_ids;

    // Extract the original sequence of chunks used for this txn from txn log
    // offsets and sequences. (This is necessary because the txn log has been
    // sorted in-place by locator at this point, and chunks may be allocated out
    // of chunk offset order.)

    // Find the lowest- and highest-allocated offset for each chunk, to
    // determine the ranges of shared memory that need to be copied into the
    // WAL. As long as the checkpointer applies versions in the same order that
    // they appear in the WAL, the final state of the checkpoint will be
    // consistent with the committed state of this txn.

    // Use a sorted vector and binary search for simplicity and cache efficiency
    // (we only sort the vector when we add a new chunk). Linear search would
    // nearly always have acceptable performance (most txns will use only one
    // chunk), but would have poor worst-case performance (a txn may use up to
    // 2^10 chunks).
    struct chunk_data_t
    {
        chunk_offset_t chunk_id;
        uint16_t min_sequence;
        gaia_offset_t min_offset;
        gaia_offset_t max_offset;
    };
    std::vector<chunk_data_t> chunk_data;

    // Obtain deleted IDs and min/max offsets per chunk.
    for (size_t i = 0; i < txn_log->record_count; ++i)
    {
        auto lr = txn_log->log_records[i];

        // Extract deleted ID and go to next record.
        if (lr.operation() == gaia_operation_t::remove)
        {
            deleted_ids.push_back(offset_to_ptr(lr.old_offset)->id);
            continue;
        }

        // Extract chunk ID from current offset.
        auto offset = lr.new_offset;
        auto sequence = lr.sequence;
        auto chunk_id = chunk_from_offset(offset);

        // Because this vector is sorted by chunk ID, we can use binary
        // search to find the index of this chunk.
        chunk_data_t target{};
        target.chunk_id = chunk_id;
        auto it = std::lower_bound(
            chunk_data.begin(), chunk_data.end(), target,
            [](const chunk_data_t& lhs, const chunk_data_t& rhs) { return lhs.chunk_id < rhs.chunk_id; });

        // If chunk isn't present, add an entry for it in sorted order.
        if (it == chunk_data.end() || it->chunk_id != chunk_id)
        {
            chunk_data.insert(it, {chunk_id, lr.sequence, offset, offset});
        }

        // Update entry for this chunk.
        auto& entry = *it;
        ASSERT_INVARIANT(entry.chunk_id == chunk_id, "Current chunk entry must store current chunk ID!");
        entry.min_offset = std::min(entry.min_offset, offset);
        entry.max_offset = std::max(entry.max_offset, offset);
        entry.min_sequence = std::min(entry.min_sequence, sequence);
    }

    // Sort chunk data by sequence rather than chunk ID, to ensure that the WAL
    // is applied in txn order.
    std::sort(
        chunk_data.begin(), chunk_data.end(),
        [](const chunk_data_t& lhs, const chunk_data_t& rhs) { return lhs.min_sequence < rhs.min_sequence; });

    // Construct an iovec for the data in each chunk to be copied to the WAL.
    std::vector<iovec> chunk_data_iovecs;

    for (const auto& entry : chunk_data)
    {
        ASSERT_INVARIANT(entry.min_offset <= entry.max_offset, "Lowest offset in chunk cannot exceed highest offset in chunk!");
        auto first_obj_ptr = offset_to_ptr(entry.min_offset);
        auto last_obj_ptr = offset_to_ptr(entry.max_offset);
        auto last_payload_size = last_obj_ptr->payload_size + c_db_object_header_size;
        size_t last_allocation_size = calculate_allocation_size_in_slots(last_payload_size) * c_slot_size_in_bytes;
        auto last_allocation_end_ptr = reinterpret_cast<std::byte*>(last_obj_ptr) + last_allocation_size;
        auto chunk_allocation_size = static_cast<size_t>(last_allocation_end_ptr - reinterpret_cast<std::byte*>(first_obj_ptr));
        chunk_data_iovecs.push_back({first_obj_ptr, chunk_allocation_size});
    }

    if (!deleted_ids.empty() || !chunk_data_iovecs.empty())
    {
        create_txn_record(commit_ts, record_type_t::txn, chunk_data_iovecs, deleted_ids);
    }
}

void log_handler_t::perform_flushed_batch_maintenance()
{
    m_async_disk_writer->perform_post_completion_maintenance();
}

void log_handler_t::submit_writes(bool should_wait_for_completion)
{
    m_async_disk_writer->submit_and_swap_in_progress_batch(m_current_file->get_file_fd(), should_wait_for_completion);
}

void log_handler_t::create_txn_record(
    gaia_txn_id_t commit_ts,
    record_type_t type,
    const std::vector<iovec>& data_iovecs,
    const std::vector<gaia_id_t>& deleted_ids)
{
    ASSERT_PRECONDITION(
        !deleted_ids.empty() || !data_iovecs.empty(),
        "Txn record cannot have empty payload.");

    std::vector<iovec> writes_to_submit;

    // Sum all iovec sizes to get initial payload size.
    size_t payload_size = std::accumulate(
        data_iovecs.begin(), data_iovecs.end(), 0,
        [](int sum, const iovec& iov) { return sum + iov.iov_len; });

    // Reserve iovec to store header for the log record.
    struct iovec header_entry = {nullptr, 0};
    writes_to_submit.push_back(header_entry);

    // Append data iovecs to submitted writes.
    writes_to_submit.insert(
        writes_to_submit.end(), data_iovecs.begin(), data_iovecs.end());

    // Augment payload size with the size of deleted ids.
    auto deleted_size = deleted_ids.size() * sizeof(gaia_id_t);
    payload_size += deleted_size;

    // Calculate total log space needed.
    auto total_log_space_needed = payload_size + sizeof(record_header_t);

    // Allocate log space.
    auto begin_offset = allocate_log_space(total_log_space_needed);

    // Create header.
    record_header_t header;
    header.crc = c_crc_initial_value;
    header.payload_size = total_log_space_needed;
    header.deleted_object_count = deleted_ids.size();
    header.txn_commit_ts = commit_ts;
    header.record_type = type;

    // Calculate CRC.
    auto txn_crc = calculate_crc32(0, &header, sizeof(record_header_t));

    // Start from 1 to skip CRC calculation for the first entry.
    for (size_t i = 1; i < writes_to_submit.size(); i++)
    {
        txn_crc = calculate_crc32(txn_crc, writes_to_submit.at(i).iov_base, writes_to_submit.at(i).iov_len);
    }

    // Augment CRC calculation with set of deleted object IDs.
    txn_crc = calculate_crc32(txn_crc, deleted_ids.data(), deleted_size);

    // Update CRC in header before sending it to the async_disk_writer.
    ASSERT_INVARIANT(txn_crc != 0, "Computed CRC cannot be zero.");
    header.crc = txn_crc;

    // Copy the header into the metadata buffer as it needs to survive the lifetime of the async_write_batch it is a part of.
    auto header_ptr = m_async_disk_writer->copy_into_metadata_buffer(&header, sizeof(record_header_t), m_current_file->get_file_fd());

    // Update the first iovec entry with the header information.
    writes_to_submit.at(0).iov_base = header_ptr;
    writes_to_submit.at(0).iov_len = sizeof(record_header_t);

    // Allocate space for deleted writes in the metadata buffer.
    if (!deleted_ids.empty())
    {
        auto deleted_id_ptr = m_async_disk_writer->copy_into_metadata_buffer(deleted_ids.data(), deleted_size, m_current_file->get_file_fd());
        writes_to_submit.push_back({deleted_id_ptr, deleted_size});
    }

    // Finally send I/O requests to the async_disk_writer.
    m_async_disk_writer->enqueue_pwritev_requests(writes_to_submit, m_current_file->get_file_fd(), begin_offset, uring_op_t::pwritev_txn);
}

} // namespace persistence
} // namespace db
} // namespace gaia
