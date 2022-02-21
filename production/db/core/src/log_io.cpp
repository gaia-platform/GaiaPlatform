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
#include <sys/stat.h>

#include "liburing.h"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/db/db_types.hpp"
#include <gaia_internal/common/mmap_helpers.hpp>
#include <gaia_internal/common/topological_sort.hpp>

#include "crc32c.h"
#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_object_helpers.hpp"
#include "log_file.hpp"
#include "mapped_data.hpp"
#include "memory_helpers.hpp"
#include "memory_types.hpp"
#include "txn_metadata.hpp"
#include "persistence_types.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;
using namespace gaia::db;
using namespace toposort;

namespace gaia
{
namespace db
{
namespace persistence
{

log_handler_t::log_handler_t(const std::string& wal_dir_path)
{
    auto dirpath = wal_dir_path;
    ASSERT_PRECONDITION(!dirpath.empty(), "Gaia persistent directory path shouldn't be empty.");
    s_wal_dir_path = dirpath.append(c_gaia_wal_dir_name);

    if (-1 == mkdir(s_wal_dir_path.c_str(), c_gaia_wal_dir_permissions) && errno != EEXIST)
    {
        throw_system_error("Unable to create persistent log directory");
    }

    if (-1 == open(s_wal_dir_path.c_str(), O_DIRECTORY))
    {
        throw_system_error("Unable to open persistent log directory.");
    }
}

void log_handler_t::open_for_writes(int validate_flushed_batch_eventfd, int signal_checkpoint_eventfd)
{
    ASSERT_PRECONDITION(validate_flushed_batch_eventfd != -1, "Eventfd to signal post flush maintenance operations invalid!");
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
        m_current_file = std::make_unique<log_file_t>(s_wal_dir_path, s_dir_fd, s_file_num, file_size);
    }
    else if (m_current_file->get_bytes_remaining_after_append(payload_size) <= 0)
    {
        m_async_disk_writer->perform_file_close_operations(m_current_file->get_file_fd(), m_current_file->get_file_sequence());

        // One batch writes to a single log file at a time.
        m_async_disk_writer->submit_and_swap_in_progress_batch(m_current_file->get_file_fd());

        m_current_file.reset();

        // Open new file.
        s_file_num++;
        auto file_size = (payload_size > c_max_log_file_size_in_bytes) ? payload_size : c_max_log_file_size_in_bytes;
        m_current_file = std::make_unique<log_file_t>(s_wal_dir_path, s_dir_fd, s_file_num, file_size);
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
    auto header_ptr = m_async_disk_writer->copy_into_metadata_buffer(&header, sizeof(record_header_t), m_current_file->get_file_fd());
    auto txn_decisions_ptr = m_async_disk_writer->copy_into_metadata_buffer(txn_decisions.data(), txn_decision_size, m_current_file->get_file_fd());

    writes_to_submit.push_back({header_ptr, sizeof(record_header_t)});
    writes_to_submit.push_back({txn_decisions_ptr, txn_decision_size});

    m_async_disk_writer->enqueue_pwritev_requests(writes_to_submit, m_current_file->get_file_fd(), m_current_file->get_current_offset(), uring_op_t::pwritev_decision);
}

void log_handler_t::create_topograph_node_helper(
    graph<chunk_offset_t>& chunk_offset_graph,
    std::unordered_map<chunk_offset_t, chunk_info_helper_t>& chunk_info_map,
    chunk_offset_t chunk_offset, 
    gaia_offset_t gaia_offset)
{
    if (chunk_info_map.count(chunk_offset) == 0)
    {
        auto node_ptr = chunk_offset_graph.create_node(std::move(chunk_offset));
        chunk_info_helper_t chunk_info_helper;
        chunk_info_helper.node_ptr = node_ptr;
        chunk_info_helper.min_offset = gaia_offset;
        chunk_info_helper.min_offset = gaia_offset;
        chunk_info_map.insert(std::pair(chunk_offset, chunk_info_helper));
    }
}

void log_handler_t::process_txn_log_and_write(int txn_log_fd, gaia_txn_id_t commit_ts)
{
    // Map in memory txn_log.
    mapped_log_t log;
    log.open(txn_log_fd);

    std::vector<common::gaia_id_t> deleted_ids;
    
    // The txn log records are sorted by locator but preserve the order of multiple updates to the same locator.
    // Use topsort to find the 'almost' original chunk order from the relative chunk ordering present in the sorted txn log.
    // If updates for a certain locator don't share chunks, then the order in which chunks are written to the persistent log
    // is irrelevant.
    // Obtaining chunk order is pretty light weight, since the number of chunks are constrained by the size of txn (defined by c_max_txn_memory_size_bytes)
    graph<chunk_offset_t> chunk_offset_graph;

    // Used to avoid dups in graph. 
    std::unordered_map<chunk_offset_t, chunk_info_helper_t> chunk_info_map;
    std::set<std::pair<chunk_offset_t, chunk_offset_t>> dependencies;

    for (size_t i = 0; i < log.data()->record_count; i++)
    {
        auto lr = log.data()->log_records[i];

        if (lr.operation() == gaia_operation_t::remove)
        {
            deleted_ids.push_back(offset_to_ptr(lr.old_offset)->id);
        }
        else
        {
            chunk_offset_t chunk_offset = memory_manager::chunk_from_offset(lr.new_offset);
            create_topograph_node_helper(chunk_offset_graph, chunk_info_map, chunk_offset, lr.new_offset);

            if (i+1 != log.data()->record_count - 1)
            {
                auto lr_plus_1 = log.data()->log_records[i + 1];
                chunk_offset_t chunk_offset_plus_1 = memory_manager::chunk_from_offset(lr_plus_1.new_offset);

                if (lr.locator == lr_plus_1.locator)
                {            
                    // Locator objects across two different chunks.
                    if (chunk_offset != chunk_offset_plus_1)       
                    {
                        // Create node.
                        create_topograph_node_helper(chunk_offset_graph, chunk_info_map, chunk_offset_plus_1, lr_plus_1.new_offset);
        
                        // Add dependency.                     
                        auto node_ptr_plus_1 = chunk_info_map.find(chunk_offset_plus_1)->second;
                        if (dependencies.count({chunk_offset_plus_1, chunk_offset}) == 0)
                        {
                            node_ptr_plus_1.node_ptr->depends_on(chunk_info_map.find(chunk_offset)->second.node_ptr);
                        }       
                    }       
                }
            }

            // Simply update min & max seen offset for the chunk.
            auto it = chunk_info_map.find(chunk_offset);
            if (lr.new_offset < it->second.min_offset)
            {
                it->second.min_offset = lr.new_offset;
            }
            else if (lr.new_offset > it->second.max_offset)
            {
                it->second.max_offset = lr.new_offset;
            }
        }
    }

    // Get chunk order.
    graph<chunk_offset_t>::node_list nodes = chunk_offset_graph.sort().sorted_nodes;

    std::vector<contiguous_offsets_t> contiguous_offsets;

    for (graph<chunk_offset_t>::node_ptr node_ptr : nodes)
    {
        auto it = chunk_info_map.find(node_ptr.get()->data);
        auto end_offset = it->second.max_offset;
        auto payload_size = offset_to_ptr(end_offset)->payload_size + c_db_object_header_size;
        size_t allocation_size = memory_manager::calculate_allocation_size_in_slots(payload_size) * c_slot_size_in_bytes;
        contiguous_offsets_t offset_pair{it->second.min_offset, end_offset + allocation_size};
        contiguous_offsets.push_back(offset_pair);
    }

    if (deleted_ids.size() > 0 || contiguous_offsets.size() > 0)
    {
        create_txn_record(commit_ts, record_type_t::txn, contiguous_offsets, deleted_ids);
    }
}

void log_handler_t::check_flushed_batch_results_and_do_maintenance()
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
    std::vector<contiguous_offsets_t>& contiguous_offsets,
    std::vector<gaia_id_t>& deleted_ids)
{
    ASSERT_PRECONDITION(!deleted_ids.empty() || !contiguous_offsets.empty(), "Txn record cannot have empty payload.");

    std::vector<iovec> writes_to_submit;

    // Reserve iovec to store header for the log record.
    struct iovec header_entry = {nullptr, 0};
    writes_to_submit.push_back(header_entry);

    // Create iovec entries.
    size_t payload_size = 0;
    for (auto offset_pair : contiguous_offsets)
    {
        auto ptr = offset_to_ptr(offset_pair.offset1);
        auto chunk_size = offset_pair.offset2 - offset_pair.offset1;
        payload_size += chunk_size;
        writes_to_submit.push_back({ptr, chunk_size});
    }

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
