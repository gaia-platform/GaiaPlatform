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

log_handler_t::log_handler_t(const std::string& wal_dir_path)
{
    auto dirpath = wal_dir_path;
    ASSERT_PRECONDITION(!dirpath.empty(), "Gaia persistent directory path shouldn't be empty.");
    s_wal_dir_path = dirpath.append(c_gaia_wal_dir_name);

    auto code = mkdir(s_wal_dir_path.c_str(), c_gaia_wal_dir_permissions);
    if (code == -1 && errno != EEXIST)
    {
        throw_system_error("Unable to create persistent log directory");
    }

    s_dir_fd = open(s_wal_dir_path.c_str(), O_DIRECTORY);
    if (s_dir_fd <= 0)
    {
        throw_system_error("Unable to open persistent log directory.");
    }
}

void log_handler_t::open_for_writes(int validate_flushed_batch_eventfd, int signal_checkpoint_eventfd)
{
    ASSERT_PRECONDITION(validate_flushed_batch_eventfd >= 0, "Invalid validate_flushed_batch_eventfd.");
    ASSERT_PRECONDITION(signal_checkpoint_eventfd >= 0, "Invalid signal_checkpoint_eventfd.");
    ASSERT_INVARIANT(s_dir_fd > 0, "Unable to open data directory for persistent log writes.");

    // Create new log file every time the log_writer gets initialized.
    m_async_disk_writer = std::make_unique<async_disk_writer_t>(validate_flushed_batch_eventfd, signal_checkpoint_eventfd);

    m_async_disk_writer->open();
}

log_handler_t::~log_handler_t()
{
    close_fd(s_dir_fd);
}

// Currently using to the rocksdb impl.
// Todo(Mihir) - Research other crc libs.
uint32_t calculate_crc32(uint32_t init_crc, void* data, size_t n)
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
    // If a transaction is greater in size than size of the log file,
    // then the entire txn is written to the next log file.
    // Another simplification is that an async_write_batch contains writes belonging to a single log file.
    if (!m_current_file)
    {
        auto file_size = (payload_size > c_file_size) ? payload_size : c_file_size;
        m_current_file.reset();
        m_current_file = std::make_unique<log_file_t>(s_wal_dir_path, s_dir_fd, s_file_num, file_size);
    }
    else if (m_current_file->get_remaining_bytes_count(payload_size) <= 0)
    {
        m_async_disk_writer->perform_file_close_operations(m_current_file->get_file_fd(), m_current_file->get_file_sequence());

        // One batch writes to a single log file at a time.
        m_async_disk_writer->submit_and_swap_in_progress_batch(m_current_file->get_file_fd());

        m_current_file.reset();

        // Open new file.
        s_file_num++;
        auto fs = (payload_size > c_file_size) ? payload_size : c_file_size;
        m_current_file = std::make_unique<log_file_t>(s_wal_dir_path, s_dir_fd, s_file_num, fs);
    }

    auto current_offset = m_current_file->get_current_offset();
    m_current_file->allocate(payload_size);

    // Return starting offset of the allocation.
    return current_offset;
}

void log_handler_t::create_decision_record(decision_list_t& txn_decisions)
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
    header.count = txn_decisions.size();
    header.txn_commit_ts = 0;
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

void log_handler_t::process_txn_log_and_write(int txn_log_fd, gaia_txn_id_t commit_ts, memory_manager_t* memory_manager)
{
    // Map in memory txn_log.
    mapped_log_t log;
    log.open(txn_log_fd);

    map_commit_ts_to_session_decision_eventfd(commit_ts, log.data()->session_decision_eventfd);

    std::vector<common::gaia_id_t> deleted_ids;
    std::map<chunk_offset_t, std::set<gaia_offset_t>> chunk_to_offsets_map;

    // Create chunk_to_offsets_map; this is done to ensure offsets are processed in the correct order.
    // The txn_log is sorted on the client for the correct validation impl, thus this pre processing is required.
    for (size_t i = 0; i < log.data()->chunk_count; i++)
    {
        auto chunk = log.data()->chunks + i;
        auto chunk_offset = memory_manager->get_chunk_offset(get_address_offset(*chunk));
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
            auto chunk = memory_manager->get_chunk_offset(get_address_offset(lr->new_offset));
            ASSERT_INVARIANT(chunk_to_offsets_map.count(chunk) > 0, "Can't find chunk.");
            ASSERT_INVARIANT(chunk != c_invalid_chunk_offset, "Invalid chunk offset found.");
            chunk_to_offsets_map.find(chunk)->second.insert(get_address_offset(lr->new_offset));
        }
    }

    // Group contiguous objects and also find total object size.
    // Each odd entry in the contiguous_offsets vector is the begin offset of a contiguous memory
    // segment and each odd entry is the ending offset of said memory chunk.
    std::vector<gaia_offset_t> contiguous_offsets;

    for (const auto& pair : chunk_to_offsets_map)
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

    // The contiguous_offsets vector should have even numb er of entries.
    ASSERT_INVARIANT(contiguous_offsets.size() % 2 == 0, "We expect a begin and end offset.");

    if (deleted_ids.size() > 0 || contiguous_offsets.size() > 0)
    {
        std::cout << "COMMIT TS - CREATE TXN RECORD = " << commit_ts << std::endl;
        create_txn_record(commit_ts, record_type_t::txn, contiguous_offsets, deleted_ids);
    }
}

void log_handler_t::map_commit_ts_to_session_decision_eventfd(gaia_txn_id_t commit_ts, int session_decision_eventfd)
{
    ASSERT_INVARIANT(session_decision_eventfd > 0, "Invalid session_decision_eventfd.");
    m_async_disk_writer->map_commit_ts_to_session_decision_efd(commit_ts, session_decision_eventfd);
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
    m_async_disk_writer->enqueue_pwritev_requests(writes_to_submit, m_current_file->get_file_fd(), m_current_file->get_current_offset(), uring_op_t::pwritev_txn);
}

void log_handler_t::register_write_to_persistent_store_fn(
    std::function<void(db_object_t&)> write_obj_fn)
{
    write_to_persistent_store_fn = write_obj_fn;
}

void log_handler_t::register_remove_from_persistent_store_fn(
    std::function<void(gaia_id_t)> remove_obj_fn)
{
    remove_from_persistent_store_fn = remove_obj_fn;
}

void log_handler_t::destroy_persistent_log(uint64_t max_log_seq_to_delete)
{
    // Done with recovery, delete all files.
    for (const auto& file : std::filesystem::directory_iterator(s_wal_dir_path))
    {
        uint64_t file_seq = std::stoi(file.path().filename());
        if (file_seq <= max_log_seq_to_delete)
        {
            std::filesystem::remove_all(file.path());
        }
    }
}

void log_handler_t::set_persistent_log_sequence(uint64_t log_seq)
{
    s_file_num = log_seq + 1;
}

void log_handler_t::recover_from_persistent_log(
    gaia_txn_id_t& last_checkpointed_commit_ts,
    uint64_t& last_processed_log_seq,
    uint64_t max_log_seq_to_process,
    recovery_mode_t mode)
{
    // Only relevant for checkpointing. Recovery doesn't care about the
    // 'last_checkpointed_commit_ts' and will reset this field to zero.
    // We don't persist txn ids across restarts.
    m_max_decided_commit_ts = last_checkpointed_commit_ts;
    std::cout << "Recovering from persistent log." << std::endl;
    // Scan all files and read log records starting from the highest numbered file.
    std::vector<uint64_t> log_files;
    for (const auto& file : std::filesystem::directory_iterator(s_wal_dir_path))
    {
        ASSERT_INVARIANT(file.is_regular_file(), "Only expecting files in persistent log directory.");
        // The file name is just the log sequence number.
        log_files.push_back(std::stoi(file.path().filename()));
    }

    // Sort files in ascending order by file name.
    sort(log_files.begin(), log_files.end());

    // Apply txns from file.
    uint64_t max_log_file_seq_to_delete = 0;
    std::vector<int> files_to_close;
    std::vector<std::pair<void*, size_t>> files_to_unmap;

    auto file_cleanup = scope_guard::make_scope_guard([&]() {
        for (auto fd : files_to_close)
        {
            close_fd(fd);
        }

        for (auto entry : files_to_unmap)
        {
            unmap_file(entry.first, entry.second);
        }
    });

    for (auto file_seq : log_files)
    {
        ASSERT_PRECONDITION(file_seq > last_processed_log_seq, "Log file sequence number should be ");
        if (file_seq > max_log_seq_to_process)
        {
            break;
        }

        // Ignore already processed files.
        if (file_seq <= last_processed_log_seq)
        {
            std::cout << "Skipping sequence = " << file_seq << std::endl;
            continue;
        }

        // Open file.
        std::stringstream file_name;
        file_name << s_wal_dir_path << "/" << file_seq;
        auto file_fd = open(file_name.str().c_str(), O_RDONLY);

        // Try to close fd as soon as possible.
        auto file_cleanup = scope_guard::make_scope_guard([&]() {
            close_fd(file_fd);
        });

        if (file_fd < 0)
        {
            throw_system_error("Unable to open persistent log file.", errno);
        }

        // mmap file.
        record_iterator_t it;
        map_log_file(&it, file_fd, mode);

        // Try to unmap file as soon as possible.
        auto mmap_cleanup = scope_guard::make_scope_guard([&]() {
            unmap_file(&it.begin, it.map_size);
        });

        // halt_recovery is set to true in case an IO issue is encountered while reading the log.
        auto halt_recovery = write_log_file_to_persistent_store(last_checkpointed_commit_ts, it);

        // Skip unmapping and closing the file in case it has some unprocessed transactions.
        if (txn_index.size() > 0)
        {
            files_to_close.push_back(file_fd);
            files_to_unmap.push_back(std::pair(it.mapped, it.map_size));
            mmap_cleanup.dismiss();
            file_cleanup.dismiss();
        }

        if (mode == recovery_mode_t::checkpoint)
        {
            if (txn_index.size() == 0)
            {
                // Safe to delete this file as it doesn't have any more txns to write to the persistent store.
                max_log_file_seq_to_delete = file_seq;
                ASSERT_INVARIANT(decision_index.size() == 0, "Failed to process all persistent log records.");
            }
        }

        if (halt_recovery)
        {
            ASSERT_INVARIANT(file_seq == log_files.back(), "We don't expect IO issues in intermediate log files.");
            break;
        }
    }

    if (log_files.size() > 0)
    {
        last_processed_log_seq = (mode == recovery_mode_t::checkpoint) ? max_log_file_seq_to_delete : log_files.back();
    }

    ASSERT_POSTCONDITION(decision_index.size() == 0, "Failed to process all persistent log records.");
}

size_t log_handler_t::get_remaining_txns_to_checkpoint_count()
{
    return txn_index.size();
}

bool log_handler_t::write_log_file_to_persistent_store(gaia_txn_id_t& last_checkpointed_commit_ts, record_iterator_t& it)
{
    // Iterate over records in file and write them to persistent store.
    write_records(&it, last_checkpointed_commit_ts);

    // Check that any remaining transactions have commit timestamp greater than the commit ts of the txn that was last written to the persistent store.
    for (auto entry : txn_index)
    {
        ASSERT_INVARIANT(entry.first > last_checkpointed_commit_ts, "Expected to find decision record for txn.");
    }

    ASSERT_POSTCONDITION(decision_index.size() == 0, "Failed to process all persistent log records in log file.");

    return it.halt_recovery;
}

void log_handler_t::write_log_record_to_persistent_store(read_record_t* record)
{
    if (record->header.record_type != record_type_t::txn)
    {
        if (record->header.record_type == record_type_t::decision)
        {
            std::cout << "RECORD TYPE WHEN WRITING TO PERSISTENT STORE = DEC" << std::endl;
        }
        else
        {
            std::cout << "RECORD TYPE WHEN WRITING TO PERSISTENT STORE = NOT SET" << std::endl;
        }
    }

    ASSERT_PRECONDITION(record->header.record_type == record_type_t::txn, "Expected transaction record.");

    auto payload_ptr = reinterpret_cast<uint8_t*>(record->payload);
    auto start_ptr = payload_ptr;
    auto end_ptr = reinterpret_cast<uint8_t*>(record) + record->header.payload_size;
    auto deleted_ids_ptr = end_ptr - (sizeof(common::gaia_id_t) * record->header.count);

    std::cout << "======= WRITING RECORD WITH TS ======= " << record->header.txn_commit_ts << " AND SIZE = " << record->header.payload_size << std::endl;
    while (payload_ptr < deleted_ids_ptr)
    {
        auto obj_ptr = reinterpret_cast<db_object_t*>(payload_ptr);

        ASSERT_INVARIANT(obj_ptr, "Object cannot be null.");
        ASSERT_INVARIANT(obj_ptr->id != common::c_invalid_gaia_id, "Recovered id cannot be invalid.");
        ASSERT_INVARIANT(obj_ptr->payload_size > 0, "Recovered object size should be greater than 0");
        write_to_persistent_store_fn(*obj_ptr);

        size_t requested_size = obj_ptr->payload_size + c_db_object_header_size;

        size_t allocation_size = base_memory_manager_t::calculate_allocation_size(requested_size);

        std::cout << "object size " << obj_ptr->payload_size << std::endl;
        std::cout << "RECORD OFFSET IN CHUNK = " << payload_ptr - start_ptr << " AND ALLOC SIZE = " << allocation_size << " AND PAYLOAD SIZE = " << requested_size << std::endl;

        // ASSERT_INVARIANT(payload_ptr + allocation_size < deleted_ids_ptr, "Object size cannot overflow outside txn record.");
        payload_ptr += allocation_size;
    }

    for (size_t i = 0; i < record->header.count; i++)
    {
        ASSERT_INVARIANT(deleted_ids_ptr < end_ptr, "Txn content overflow.");
        auto deleted_id = reinterpret_cast<common::gaia_id_t*>(deleted_ids_ptr);
        ASSERT_INVARIANT(deleted_id, "Deleted ID cannot be null.");
        ASSERT_INVARIANT(*deleted_id > 0, "Deleted ID cannot be invalid.");
        remove_from_persistent_store_fn(*deleted_id);
        deleted_ids_ptr += sizeof(common::gaia_id_t);
    }
}

void log_handler_t::write_records(record_iterator_t* it, gaia_txn_id_t& last_checkpointed_commit_ts)
{
    size_t record_size = 0;

    do
    {
        auto current_record_ptr = it->cursor;
        record_size = update_cursor(it);

        if (record_size == 0)
        {
            if (it->halt_recovery || it->cursor >= it->stop_at)
            {
                it->cursor = nullptr;
                it->end = nullptr;
                break;
            }

            ASSERT_INVARIANT(it->halt_recovery, "We don't expect empty records to be logged.");
        }

        // Todo: Don't cast structs directly, instead decode byte by byte.
        read_record_t* record = reinterpret_cast<read_record_t*>(current_record_ptr);

        if (record_size != 0 && record->header.record_type == record_type_t::decision)
        {
            // Decode decision record.
            auto payload_ptr = current_record_ptr + sizeof(record_header_t);

            // Obtain decisions. Decisions may not be in commit order, so sort and process them.
            for (size_t i = 0; i < record->header.count; i++)
            {
                auto decision_entry = reinterpret_cast<decision_entry_t*>(payload_ptr);
                if (decision_entry->commit_ts > last_checkpointed_commit_ts)
                {
                    ASSERT_INVARIANT(txn_index.count(decision_entry->commit_ts) > 0, "Transaction record should be written before the decision record.");
                    decision_index.insert(std::pair(decision_entry->commit_ts, decision_entry->decision));
                }
                payload_ptr += sizeof(decision_entry_t);
            }

            // Iterare decisions.
            for (auto decision_it = decision_index.cbegin(); decision_it != decision_index.cend();)
            {
                std::cout << "COMMIT TS FOR DEC IS = " << decision_it->first << std::endl;
                ASSERT_INVARIANT(txn_index.count(decision_it->first) > 0, "Transaction record should be written before the decision record.");

                auto txn_it = txn_index.find(decision_it->first);

                // Only perform recovery and checkpointing for committed transactions.
                if (decision_it->second == decision_type_t::commit)
                {
                    // Txn record is safe to be written to rocksdb at this point, since checksums for both
                    // the txn & decision record were validated and we asserted that the txn record is written
                    // before the decision record in the wal.
                    write_log_record_to_persistent_store(reinterpret_cast<read_record_t*>(txn_it->second));
                }

                // Update 'last_checkpointed_commit_ts' in memory so it can later be written to persistent store.
                last_checkpointed_commit_ts = std::max(last_checkpointed_commit_ts, decision_it->first);
                std::cout << "Erasing decision = " << decision_it->first << std::endl;
                txn_it = txn_index.erase(txn_it);
                decision_it = decision_index.erase(decision_it);
            }
        }
        else if (record_size != 0 && record->header.record_type == record_type_t::txn)
        {
            // Skip over records that have already been checkpointed.
            if (record->header.txn_commit_ts <= last_checkpointed_commit_ts)
            {
                continue;
            }
            txn_index.insert(std::pair(record->header.txn_commit_ts, current_record_ptr));
        }

    } while (true);
}

size_t log_handler_t::update_cursor(struct record_iterator_t* it)
{
    if (it->cursor < it->stop_at)
    {
        size_t record_size = validate_recovered_record_crc(it);

        // Recovery failure.
        if (record_size == 0)
        {
            return 0;
        }

        it->cursor += record_size;
        return record_size;
    }
    else
    {
        return 0;
    }
}

size_t log_handler_t::validate_recovered_record_crc(struct record_iterator_t* it)
{
    auto destination = reinterpret_cast<read_record_t*>(it->cursor);
    std::cout << "RECOVERY: CURSOR = " << it->cursor - it->begin << " AND RECORD = " << (uint8_t)destination->header.record_type << std::endl;

    if (destination->header.payload_size == 0)
    {
        std::cout << "CRC = " << destination->header.crc << std::endl;
        std::cout << "PAYLOAD SIZE = " << destination->header.payload_size << std::endl;
        std::cout << "COMMIT TS = " << destination->header.txn_commit_ts << std::endl;
        std::cout << "halt here " << std::endl;
    }
    if (destination->header.crc == 0)
    {
        std::cout << "halt here 2" << std::endl;
        if (is_remaining_file_empty(it->cursor, it->end))
        {
            // Stop processing the current log file.
            it->stop_at = it->cursor;
            return 0;
        }

        std::cout << "HEADER CRC zero." << std::endl;
        if (it->recovery_mode == recovery_mode_t::checkpoint)
        {
            throw write_ahead_log_error("Read log record with empty checksum value.");
        }
        else if (it->recovery_mode == recovery_mode_t::finish_on_first_error)
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
        std::cout << "CRC mismatch." << std::endl;
        if (it->recovery_mode == recovery_mode_t::checkpoint)
        {
            throw write_ahead_log_error("Record checksum match failed!");
        }

        if (it->recovery_mode == recovery_mode_t::finish_on_first_error)
        {
            it->halt_recovery = true;
            return 0;
        }
    }

    return header_copy.payload_size;
}

void log_handler_t::map_log_file(struct record_iterator_t* it, int file_fd, recovery_mode_t recovery_mode)
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
        0);

    *it = (struct record_iterator_t){
        .cursor = (uint8_t*)wal_record,
        .end = (uint8_t*)wal_record + st.st_size,
        .stop_at = (uint8_t*)wal_record + st.st_size,
        .begin = (uint8_t*)wal_record,
        .mapped = wal_record,
        .map_size = (size_t)st.st_size,
        .file_fd = file_fd,
        .recovery_mode = recovery_mode,
        .halt_recovery = false};
}

bool log_handler_t::is_remaining_file_empty(uint8_t* start, uint8_t* end)
{
    auto remaining_size = end - start;
    uint8_t zeroblock[remaining_size];
    memset(zeroblock, 0, sizeof zeroblock);
    return memcmp(zeroblock, start, remaining_size) == 0;
}

void log_handler_t::unmap_file(void* begin, size_t size)
{
    munmap(begin, size);
}

} // namespace persistence
} // namespace db
} // namespace gaia
