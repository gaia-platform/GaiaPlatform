/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "log_io.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <charconv>
#include <cstddef>

#include <atomic>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <string>

#include "liburing.h"

#include "gaia_internal/common/retail_assert.hpp"
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
#include "persistent_store_manager.hpp"
#include "rdb_object_converter.hpp"
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
    ASSERT_PRECONDITION(validate_flushed_batch_eventfd != -1, "Invalid eventfd!");
    ASSERT_PRECONDITION(signal_checkpoint_eventfd != -1, "Invalid eventfd!");
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
        auto file_size = (payload_size > c_file_size) ? payload_size : c_file_size;
        m_current_file.reset();
        m_current_file = std::make_unique<log_file_t>(s_wal_dir_path, s_dir_fd, s_file_num.load(), file_size);
    }
    else if (m_current_file->get_bytes_remaining_after_append(payload_size) <= 0)
    {
        m_async_disk_writer->perform_file_close_operations(m_current_file->get_file_fd(), m_current_file->get_log_sequence());

        // One batch writes to a single log file at a time.
        m_async_disk_writer->submit_and_swap_in_progress_batch(m_current_file->get_file_fd());

        m_current_file.reset();

        // Open new file.
        s_file_num++;
        auto fs = (payload_size > c_file_size) ? payload_size : c_file_size;
        m_current_file = std::make_unique<log_file_t>(s_wal_dir_path, s_dir_fd, s_file_num.load(), fs);
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
    size_t txn_decision_size = txn_decisions.size() * (sizeof(gaia_txn_id_t) + sizeof(decision_type_t));
    auto total_log_space_needed = txn_decision_size + sizeof(record_header_t);
    allocate_log_space(total_log_space_needed);

    // Create log record header.
    record_header_t header;
    header.crc = c_crc_initial_value;
    header.payload_size = total_log_space_needed;
    header.decision_count = txn_decisions.size();
    header.txn_commit_ts = c_invalid_gaia_txn_id;
    header.record_type = record_type_t::decision;

    // Compute crc.
    crc32_t txn_crc = 0;
    txn_crc = calculate_crc32(txn_crc, &header, sizeof(record_header_t));
    txn_crc = calculate_crc32(txn_crc, txn_decisions.data(), txn_decision_size);

    ASSERT_INVARIANT(txn_crc > 0, "CRC cannot be zero.");
    header.crc = txn_crc;

    // Copy information which needs to survive for the entire batch lifetime into the metadata buffer.
    auto header_ptr = m_async_disk_writer->copy_into_metadata_buffer(&header, sizeof(record_header_t), m_current_file->get_file_fd());
    auto txn_decisions_ptr = m_async_disk_writer->copy_into_metadata_buffer(txn_decisions.data(), txn_decision_size, m_current_file->get_file_fd());

    writes_to_submit.push_back({header_ptr, sizeof(record_header_t)});
    writes_to_submit.push_back({txn_decisions_ptr, txn_decision_size});

    m_async_disk_writer->enqueue_pwritev_requests(writes_to_submit, m_current_file->get_file_fd(), m_current_file->get_current_offset(), uring_op_t::pwritev_decision);
}

void log_handler_t::process_txn_log_and_write(int txn_log_fd, gaia_txn_id_t commit_ts)
{
    // Map in memory txn_log.
    mapped_log_t log;
    log.open(txn_log_fd);

    register_commit_ts_for_session_notification(commit_ts, log.data()->session_decision_eventfd);

    std::vector<common::gaia_id_t> deleted_ids;

    // Create chunk_to_offsets_map; this is done to ensure offsets are processed in the correct order.
    // The txn_log is sorted on the client for the correct validation impl, thus this map is used to track order of writes.
    // Note that writes beloning to a txn can be assigned in arbitrary chunk order (due to chunk reuse) which is another reason to
    // track chunk ids in the log.
    std::map<chunk_offset_t, std::set<gaia_offset_t>> chunk_to_offsets_map;
    for (size_t i = 0; i < log.data()->chunk_count; i++)
    {
        auto chunk = log.data()->chunks + i;
        auto chunk_offset = memory_manager::chunk_from_offset(*chunk);
        chunk_to_offsets_map.insert(std::pair(chunk_offset, std::set<gaia_offset_t>()));
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
            auto chunk = memory_manager::chunk_from_offset(lr->new_offset);
            ASSERT_INVARIANT(chunk_to_offsets_map.count(chunk) > 0, "Can't find chunk.");
            ASSERT_INVARIANT(chunk != c_invalid_chunk_offset, "Invalid chunk offset found.");
            chunk_to_offsets_map.find(chunk)->second.insert(lr->new_offset);
        }
    }

    // Group contiguous objects and also find total object size.
    // Each odd entry in the contiguous_offsets vector is the begin offset of a contiguous memory
    // segment and each even entry is the ending offset of said memory chunk.
    std::vector<gaia_offset_t> contiguous_offsets;

    for (const auto& pair : chunk_to_offsets_map)
    {
        auto object_address_offsets = pair.second;
        if (object_address_offsets.size() == 0)
        {
            continue;
        }
        contiguous_offsets.push_back(*object_address_offsets.begin());
        auto end_offset = *(--object_address_offsets.end());
        auto payload_size = offset_to_ptr(end_offset)->payload_size + c_db_object_header_size;
        size_t allocation_size = memory_manager::calculate_allocation_size_in_slots(payload_size) * c_slot_size_in_bytes;
        contiguous_offsets.push_back(end_offset + allocation_size);
    }

    // The contiguous_offsets vector should have an even number of entries.
    ASSERT_INVARIANT(contiguous_offsets.size() % 2 == 0, "We expect a begin and end offset.");

    if (deleted_ids.size() > 0 || contiguous_offsets.size() > 0)
    {
        create_txn_record(commit_ts, record_type_t::txn, contiguous_offsets, deleted_ids);
    }
}

void log_handler_t::register_commit_ts_for_session_notification(gaia_txn_id_t commit_ts, int session_decision_eventfd)
{
    ASSERT_INVARIANT(session_decision_eventfd > 0, "Invalid session_decision_eventfd.");
    m_async_disk_writer->map_commit_ts_to_session_decision_eventfd(commit_ts, session_decision_eventfd);
}

void log_handler_t::validate_flushed_batch()
{
    m_async_disk_writer->perform_post_completion_maintenance();
}

void log_handler_t::submit_writes(bool sync)
{
    m_async_disk_writer->submit_and_swap_in_progress_batch(m_current_file->get_file_fd(), sync);
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

    // Create iovec entries for txn objects.
    size_t payload_size = 0;
    for (size_t i = 0; i < contiguous_address_offsets.size(); i += 2)
    {
        auto offset = contiguous_address_offsets.at(i);
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
    ASSERT_INVARIANT(txn_crc > 0, "Computed CRC cannot be zero.");
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

void log_handler_t::truncate_persistent_log(log_sequence_t max_log_seq_to_delete)
{
    // Done with recovery/checkpointing, delete all files with sequence number <=
    // max_log_seq_to_delete
    for (const auto& file : std::filesystem::directory_iterator(s_wal_dir_path))
    {
        auto filename_str = file.path().filename().string();

        // Unable to use log_sequence_t here since the from_chars() method only handles int types.
        size_t log_seq = c_invalid_log_sequence_number;
        const auto result = std::from_chars(filename_str.c_str(), filename_str.c_str() + filename_str.size(), log_seq);
        if (result.ec != std::errc{})
        {
            throw_system_error("Failed to convert log file name to integer!");
        }

        if (log_seq <= max_log_seq_to_delete)
        {
            // Throws filesystem::filesystem_error on underlying OS API errors.
            // TODO: Revisit whether failure of the filesystem should cause the server to crash.
            std::filesystem::remove(file.path());
        }
    }
}

void log_handler_t::set_persistent_log_sequence(log_sequence_t log_seq)
{
    // Extremely unlikely but add a check anyway.
    ASSERT_PRECONDITION(log_seq != std::numeric_limits<uint64_t>::max(), "Log file sequence number is exhausted!");
    s_file_num = log_seq + 1;
}

void log_handler_t::recover_from_persistent_log(
    std::shared_ptr<persistent_store_manager_t>& s_persistent_store,
    gaia_txn_id_t& last_checkpointed_commit_ts,
    log_sequence_t& last_processed_log_seq,
    log_sequence_t max_log_seq_to_process,
    log_reader_mode_t mode)
{
    // Only relevant for checkpointing. Recovery doesn't care about the
    // 'last_checkpointed_commit_ts' and will reset this field to zero.
    // We don't persist txn ids across restarts.
    m_max_decided_commit_ts = last_checkpointed_commit_ts;

    // Scan all files and read log records starting from the highest numbered file.
    std::vector<log_sequence_t> log_file_sequences;
    for (const auto& file : std::filesystem::directory_iterator(s_wal_dir_path))
    {
        ASSERT_INVARIANT(file.is_regular_file(), "Only expecting files in persistent log directory.");

        auto filename_str = file.path().filename().string();

        // The file name is just the log sequence number.
        size_t log_seq = c_invalid_log_sequence_number;
        const auto result = std::from_chars(filename_str.c_str(), filename_str.c_str() + filename_str.size(), log_seq);
        if (result.ec != std::errc{})
        {
            throw_system_error("Failed to convert log file name to integer!");
        }
        log_file_sequences.push_back(log_seq);
    }

    // Sort files in ascending order by file name.
    std::sort(log_file_sequences.begin(), log_file_sequences.end());

    // Apply txns from file.
    log_sequence_t max_log_seq_to_delete = 0;
    std::vector<int> file_fds_to_close;
    std::vector<log_file_pointer_t> file_pointers_to_unmap;

    auto file_cleanup = scope_guard::make_scope_guard([&]() {
        for (auto fd : file_fds_to_close)
        {
            close_fd(fd);
        }

        for (log_file_pointer_t entry : file_pointers_to_unmap)
        {
            unmap_fd_data(entry.begin, entry.size);
        }
    });

    for (log_sequence_t log_seq : log_file_sequences)
    {
        if (log_seq > max_log_seq_to_process)
        {
            break;
        }

        // Ignore already processed files.
        if (log_seq <= last_processed_log_seq)
        {
            continue;
        }

        // Open file.
        std::filesystem::path file_name = s_wal_dir_path / std::string(log_seq.value);
        auto file_fd = open(file_name.c_str(), O_RDONLY);

        if (file_fd == -1)
        {
            throw_system_error("Unable to open persistent log file.");
        }

        // Try to close fd as soon as possible.
        auto file_cleanup = scope_guard::make_scope_guard([&]() {
            close_fd(file_fd);
        });

        // mmap file.
        record_iterator_t it;
        map_log_file(&it, file_fd, mode);

        // Try to unmap file as soon as possible.
        auto mmap_cleanup = scope_guard::make_scope_guard([&]() {
            unmap_fd_data(it.begin, it.map_size);
        });

        // halt_recovery is set to true if an IO error is encountered while reading the log.
        bool halt_recovery = write_log_file_to_persistent_store(s_persistent_store, &it, &last_checkpointed_commit_ts);

        // Skip unmapping and closing the file if it has some unprocessed transactions.
        if (m_txn_records_by_commit_ts.size() > 0)
        {
            file_fds_to_close.push_back(file_fd);
            file_cleanup.dismiss();

            file_pointers_to_unmap.push_back({it.mapped_data, it.map_size});
            mmap_cleanup.dismiss();
        }
        else if (mode == log_reader_mode_t::checkpoint_fail_on_first_error)
        {
            if (m_txn_records_by_commit_ts.size() == 0)
            {
                // Safe to delete this file as it doesn't have any more txns to write to the persistent store.
                max_log_seq_to_delete = log_seq;
                ASSERT_INVARIANT(m_decision_records_by_commit_ts.size() == 0, "Failed to process all persistent log records.");
            }
        }

        if (halt_recovery)
        {
            // TODO: THROW an error based on recovery mode.

            // TODO: Verify whether it is possible to see IO errors in intermediate log files.
            // ASSERT_INVARIANT(log_seq == log_file_sequences.back(), "We don't expect IO errors in intermediate log files.");
            break;
        }
    }

    if (log_file_sequences.size() > 0)
    {
        // Set last_processed_log_seq as max_log_seq_to_delete in Checkpoint mode & to the last
        // observed log sequence in the wal directory during recovery.
        last_processed_log_seq = (mode == log_reader_mode_t::checkpoint_fail_on_first_error) ? max_log_seq_to_delete : log_file_sequences.back();
    }

    ASSERT_POSTCONDITION(m_decision_records_by_commit_ts.size() == 0, "Failed to process all persistent log records.");
}

bool log_handler_t::write_log_file_to_persistent_store(
    std::shared_ptr<persistent_store_manager_t>& persistent_store_manager,
    record_iterator_t* it,
    gaia_txn_id_t* last_checkpointed_commit_ts)
{
    // Iterate over records in file and write them to persistent store.
    write_records(persistent_store_manager, it, last_checkpointed_commit_ts);

    // Check that any remaining transactions have commit timestamp greater than the commit ts of the
    // txn that was last written to the persistent store.
    for (auto entry : m_txn_records_by_commit_ts)
    {
        ASSERT_INVARIANT(entry.first > *last_checkpointed_commit_ts, "Expected txn to be checkpointed!");
    }

    // The log file should not have any decision records that are yet to be processed.
    ASSERT_POSTCONDITION(!it->halt_recovery && m_decision_records_by_commit_ts.size() == 0, "Failed to process all persistent log records in log file.");

    return it->halt_recovery;
}

void log_handler_t::write_log_record_to_persistent_store(
    std::shared_ptr<persistent_store_manager_t>& persistent_store_manager,
    read_record_t* record)
{
    ASSERT_PRECONDITION(record->header.record_type == record_type_t::txn, "Expected transaction record.");

    auto payload_ptr = reinterpret_cast<unsigned char*>(record->payload);
    auto start_ptr = payload_ptr;
    auto end_ptr = reinterpret_cast<unsigned char*>(record) + record->header.payload_size;
    auto deleted_ids_ptr = end_ptr - (sizeof(common::gaia_id_t) * record->header.deleted_object_count);

    while (payload_ptr < deleted_ids_ptr)
    {
        // We use string_reader_t here to read the object instead of directly using
        // reinterpret_cast<db_object_t*> since Gaia objects are packed into the log and don't
        // respect the alignment requirement for db_object_t. This is done for the sake of
        // encoding simplicity and to avoid bloating the size of the log. (Although the
        // latter isn't a very strong argument)
        // The object metadata occupies 16 bytes: 8 (id) + 4 (type) + 2 (payload_size)
        // + 2 (num_references) = 16.
        auto obj_ptr = payload_ptr;
        ASSERT_INVARIANT(obj_ptr, "Object cannot be null.");

        gaia_id_t id = *reinterpret_cast<gaia_id_t*>(obj_ptr);
        ASSERT_INVARIANT(id != common::c_invalid_gaia_id, "Recovered id cannot be invalid.");
        obj_ptr += sizeof(gaia_id_t);

        gaia_type_t type = *reinterpret_cast<gaia_type_t*>(obj_ptr);
        ASSERT_INVARIANT(type != common::c_invalid_gaia_type, "Recovered type cannot be invalid.");
        obj_ptr += sizeof(gaia_type_t);

        uint16_t payload_size = *reinterpret_cast<uint16_t*>(obj_ptr);
        ASSERT_INVARIANT(payload_size > 0, "Recovered object size should be greater than 0");
        obj_ptr += sizeof(uint16_t);

        reference_offset_t num_references = *reinterpret_cast<reference_offset_t*>(obj_ptr);
        obj_ptr += sizeof(reference_offset_t);

        auto references = reinterpret_cast<const gaia_id_t*>(obj_ptr);
        auto data = reinterpret_cast<const char*>(obj_ptr) + num_references * sizeof(gaia::common::gaia_id_t);

        persistent_store_manager->put(
            id,
            type,
            num_references,
            references,
            payload_size,
            payload_size - (num_references * sizeof(gaia::common::gaia_id_t)),
            data);

        size_t requested_size = payload_size + c_db_object_header_size;

        size_t allocation_size = memory_manager::calculate_allocation_size_in_slots(requested_size) * c_slot_size_in_bytes;

        payload_ptr += allocation_size;
    }

    for (size_t i = 0; i < record->header.deleted_object_count; i++)
    {
        ASSERT_INVARIANT(deleted_ids_ptr < end_ptr, "Txn content overflow.");
        auto deleted_id = reinterpret_cast<common::gaia_id_t*>(deleted_ids_ptr);
        ASSERT_INVARIANT(deleted_id, "Deleted ID cannot be null.");
        ASSERT_INVARIANT(*deleted_id > 0, "Deleted ID cannot be invalid.");
        persistent_store_manager->remove(*deleted_id);
        deleted_ids_ptr += sizeof(common::gaia_id_t);
    }
}

void log_handler_t::write_records(
    std::shared_ptr<persistent_store_manager_t>& persistent_store_manager,
    record_iterator_t* it,
    gaia_txn_id_t* last_checkpointed_commit_ts)
{
    size_t record_size = 0;

    do
    {
        auto current_record_ptr = it->iterator;
        record_size = update_iterator(it);

        if (record_size == 0)
        {
            if (it->halt_recovery || it->iterator >= it->stop_at)
            {
                it->iterator = nullptr;
                it->end = nullptr;
                break;
            }

            ASSERT_INVARIANT(it->halt_recovery, "We don't expect empty records to be logged.");
        }

        read_record_t* record = reinterpret_cast<read_record_t*>(current_record_ptr);

        if (record_size != 0 && record->header.record_type == record_type_t::decision)
        {
            // Decode decision record.
            auto payload_ptr = current_record_ptr + sizeof(record_header_t);

            // Obtain decisions. Decisions may not be in commit order, so sort and process them.
            for (size_t i = 0; i < record->header.decision_count; i++)
            {
                auto decision_entry = reinterpret_cast<decision_entry_t*>(payload_ptr);
                if (decision_entry->commit_ts > *last_checkpointed_commit_ts)
                {
                    ASSERT_INVARIANT(m_txn_records_by_commit_ts.count(decision_entry->commit_ts) > 0, "Transaction record should be written before the decision record.");
                    m_decision_records_by_commit_ts.insert(std::pair(decision_entry->commit_ts, decision_entry->decision));
                }
                payload_ptr += sizeof(decision_entry_t);
            }

            // Iterare decisions.
            for (auto decision_it = m_decision_records_by_commit_ts.cbegin(); decision_it != m_decision_records_by_commit_ts.cend();)
            {
                ASSERT_INVARIANT(m_txn_records_by_commit_ts.count(decision_it->first) > 0, "Transaction record should be written before the decision record.");

                auto txn_it = m_txn_records_by_commit_ts.find(decision_it->first);

                // Only perform recovery and checkpointing for committed transactions.
                if (decision_it->second == decision_type_t::commit)
                {
                    // Txn record is safe to be written to rocksdb at this point, since checksums for both
                    // the txn & decision record were validated and we asserted that the txn record is written
                    // before the decision record in the wal.
                    write_log_record_to_persistent_store(persistent_store_manager, reinterpret_cast<read_record_t*>(txn_it->second));
                }

                // Update 'last_checkpointed_commit_ts' in memory so it can later be written to persistent store.
                *last_checkpointed_commit_ts = std::max(*last_checkpointed_commit_ts, decision_it->first);
                txn_it = m_txn_records_by_commit_ts.erase(txn_it);
                decision_it = m_decision_records_by_commit_ts.erase(decision_it);
            }
        }
        else if (record_size != 0 && record->header.record_type == record_type_t::txn)
        {
            // Skip over records that have already been checkpointed.
            if (record->header.txn_commit_ts <= *last_checkpointed_commit_ts)
            {
                continue;
            }
            m_txn_records_by_commit_ts.insert(std::pair(record->header.txn_commit_ts, current_record_ptr));
        }

    } while (true);
}

size_t log_handler_t::update_iterator(struct record_iterator_t* it)
{
    if (it->iterator < it->stop_at)
    {
        size_t record_size = validate_recovered_record_checksum(it);

        // Recovery failure.
        if (record_size == 0)
        {
            return 0;
        }

        it->iterator += record_size;
        return record_size;
    }
    else
    {
        return 0;
    }
}

size_t log_handler_t::validate_recovered_record_checksum(struct record_iterator_t* it)
{
    auto destination = reinterpret_cast<read_record_t*>(it->iterator);

    if (destination->header.crc == 0)
    {
        if (is_remaining_file_empty(it->iterator, it->end))
        {
            // Stop processing the current log file.
            it->stop_at = it->iterator;
            return 0;
        }

        if (it->recovery_mode == log_reader_mode_t::checkpoint_fail_on_first_error)
        {
            throw std::runtime_error("Log record has empty checksum value!");
        }
        else if (it->recovery_mode == log_reader_mode_t::recovery_stop_on_first_error)
        {
            // We expect all log files to be truncated to appropriate size, so halt recovery.
            it->halt_recovery = true;
            return 0;
        }
        else
        {
            ASSERT_INVARIANT(false, "Unexpected recovery mode.");
        }
    }

    // CRC calculation requires manipulating the recovered record header's CRC;
    // So create a copy of the header to avoid modifying the wal log once it has been written.
    record_header_t header_copy;
    memcpy(&header_copy, &destination->header, sizeof(record_header_t));

    auto expected_crc = header_copy.crc;
    header_copy.crc = c_crc_initial_value;

    // First calculate CRC of header.
    auto crc = calculate_crc32(0, &header_copy, sizeof(record_header_t));

    // Calculate payload CRC.
    crc = calculate_crc32(crc, destination->payload, header_copy.payload_size - sizeof(record_header_t));

    if (crc != expected_crc)
    {
        if (it->recovery_mode == log_reader_mode_t::checkpoint_fail_on_first_error)
        {
            throw std::runtime_error("Record checksum match failed!");
        }

        if (it->recovery_mode == log_reader_mode_t::recovery_stop_on_first_error)
        {
            it->halt_recovery = true;
            return 0;
        }
    }

    return header_copy.payload_size;
}

void log_handler_t::map_log_file(struct record_iterator_t* it, int file_fd, log_reader_mode_t recovery_mode)
{
    struct stat st;
    read_record_t* wal_record;

    if (fstat(file_fd, &st) == -1)
    {
        throw_system_error("failed to fstat wal file");
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
        0);

    *it = (struct record_iterator_t){
        .iterator = (unsigned char*)wal_record,
        .end = (unsigned char*)wal_record + st.st_size,
        .stop_at = (unsigned char*)wal_record + st.st_size,
        .begin = (unsigned char*)wal_record,
        .mapped_data = wal_record,
        .map_size = (size_t)st.st_size,
        .file_fd = file_fd,
        .recovery_mode = recovery_mode,
        .halt_recovery = false};
}

bool log_handler_t::is_remaining_file_empty(unsigned char* start, unsigned char* end)
{
    auto remaining_size = end - start;
    unsigned char zeroblock[remaining_size];
    memset(zeroblock, 0, sizeof zeroblock);
    return memcmp(zeroblock, start, remaining_size) == 0;
}

} // namespace persistence
} // namespace db
} // namespace gaia
