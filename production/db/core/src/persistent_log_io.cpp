/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "persistent_log_io.hpp"

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
#include "mapped_data.hpp"
#include "memory_manager.hpp"
#include "memory_types.hpp"
#include "persistent_log_file.hpp"
#include "txn_metadata.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;
using namespace gaia::db;

namespace gaia
{
namespace db
{

persistent_log_handler_t::persistent_log_handler_t(const std::string& directory_path)
{
    std::cout << "Creating gaia wal dir" << std::endl;
    auto dirpath = directory_path;
    ASSERT_PRECONDITION(!dirpath.empty(), "Gaia persistent directory path shouldn't be empty.");
    s_wal_dir_path = dirpath.append(c_gaia_wal_dir_name);
    std::cout << "WAL Directory is = " << s_wal_dir_path << std::endl;
    auto code = mkdir(s_wal_dir_path.c_str(), 0755);
    if (code == -1 && errno != EEXIST)
    {
        throw_system_error("Unable to create persistent log directory");
    }
    dir_fd = open(s_wal_dir_path.c_str(), O_DIRECTORY);
    if (dir_fd <= 0)
    {
        throw_system_error("Unable to open persistent log directory.");
    }

    max_decided_commit_ts = c_invalid_gaia_txn_id;
}

void persistent_log_handler_t::open_for_writes(int validate_flushed_batch_efd)
{
    ASSERT_PRECONDITION(validate_flushed_batch_efd >= 0, "Invalid validate flush eventfd.");
    ASSERT_INVARIANT(dir_fd > 0, "Unable to open data directory for persistent log writes.");

    // Create new wal file every time the wal writer gets initialized.
    async_disk_writer = std::make_unique<async_disk_writer_t>(validate_flushed_batch_efd);

    auto set_txn_durable_fn = [=](gaia_txn_id_t commit_ts) {
        txn_metadata_t::set_txn_durable(commit_ts);
    };
    async_disk_writer->register_txn_durable_fn(set_txn_durable_fn);

    async_disk_writer->open();
}

void persistent_log_handler_t::register_write_to_persistent_store_fn(
    std::function<void(db_object_t&)> write_obj_fn)
{
    write_to_persistent_store_fn = write_obj_fn;
}

void persistent_log_handler_t::register_remove_from_persistent_store_fn(
    std::function<void(gaia_id_t)> remove_obj_fn)
{
    remove_from_persistent_store_fn = remove_obj_fn;
}

persistent_log_handler_t::~persistent_log_handler_t()
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

persistent_log_file_offset_t persistent_log_handler_t::allocate_log_space(size_t payload_size)
{
    // For simplicity, we don't break up transaction records across log files. We simply
    // write it to the next log file. If a transaction is greater in size than size of the log file,
    // then we write out the entire txn in the new log file. In this case log files.
    // TODO: ASSERT(payload_size < log_file_size)
    if (!current_file)
    {
        std::cout << "Current file doesn't exist" << std::endl;
        auto fs = (payload_size > c_file_size) ? payload_size : c_file_size;
        current_file.reset(new persistent_log_file_t(s_wal_dir_path, dir_fd, file_num, fs));
    }
    else if (!current_file->has_enough_space(payload_size))
    {
        async_disk_writer->handle_file_close(current_file->get_file_fd(), current_file->get_current_offset());

        // As a simplification, one batch writes to a single log file at a time.
        async_disk_writer->handle_submit(current_file->get_file_fd());

        current_file.reset();

        // Open new file.
        file_num++;
        auto fs = (payload_size > c_file_size) ? payload_size : c_file_size;
        std::cout << "File out of space; creating new file with fs = " << fs << std::endl;
        current_file = std::make_unique<persistent_log_file_t>(s_wal_dir_path, dir_fd, file_num, fs);
    }
    persistent_log_file_offset_t current_offset = current_file->get_current_offset();
    current_file->allocate(payload_size);
    return current_offset;
}

void persistent_log_handler_t::create_decision_record(decision_list_t& txn_decisions)
{
    check_endianness();
    ASSERT_PRECONDITION(!txn_decisions.empty(), "Decision record cannot have empty payload.");
    // Track decisions per batch.
    async_disk_writer->add_decisions_to_batch(txn_decisions);

    // Create decision record and write it using pwritev.
    std::vector<iovec> writes_to_submit;
    size_t txn_decision_size = txn_decisions.size() * (sizeof(gaia_txn_id_t) + sizeof(txn_decision_type_t));
    auto total_log_space_needed = txn_decision_size + sizeof(record_header_t);
    persistent_log_file_offset_t begin_log_offset = allocate_log_space(total_log_space_needed);

    record_header_t header;
    header.crc = c_crc_initial_value;
    header.payload_size = total_log_space_needed;
    header.count = txn_decisions.size();
    header.txn_commit_ts = 0;
    header.record_type = record_type_t::decision;

    // std::cout << "DECISION RECORD OFFSET = " << begin_log_offset << " AND SIZE = " << total_log_space_needed << std::endl;

    crc32_t txn_crc = 0;
    txn_crc = calculate_crc32(txn_crc, &header, sizeof(record_header_t));
    txn_crc = calculate_crc32(txn_crc, txn_decisions.data(), txn_decision_size);

    ASSERT_INVARIANT(txn_crc > 0, "CRC cannot be zero.");
    header.crc = txn_crc;

    // std::cout << "DECISION HDR" << std::endl;
    auto header_ptr = async_disk_writer->copy_into_metadata_buffer(&header, sizeof(record_header_t), current_file->get_file_fd());
    // std::cout << "DECISIONS" << std::endl;
    auto txn_decisions_ptr = async_disk_writer->copy_into_metadata_buffer(txn_decisions.data(), txn_decision_size, current_file->get_file_fd());

    writes_to_submit.push_back({header_ptr, sizeof(record_header_t)});
    writes_to_submit.push_back({txn_decisions_ptr, txn_decision_size});

    async_disk_writer->construct_pwritev(writes_to_submit, current_file->get_file_fd(), begin_log_offset, current_file->get_current_offset(), uring_op_t::PWRITEV_DECISION);
}

void persistent_log_handler_t::process_txn_log_and_write(int txn_log_fd, gaia_txn_id_t commit_ts, memory_manager_t* memory_manager)
{
    mapped_log_t log;
    log.open(txn_log_fd);

    map_commit_ts_to_session_unblock_fd(commit_ts, log.data()->session_unblock_fd);

    std::cout << "CREATE TXN RECORD COMMIT TS =" << commit_ts << std::endl;
    std::vector<common::gaia_id_t> deleted_ids;
    std::map<chunk_offset_t, std::set<gaia_offset_t>> map;

    // Init map.
    for (size_t i = 0; i < log.data()->chunk_count; i++)
    {
        auto chunk = log.data()->chunks + i;
        auto chunk_offset = memory_manager->get_chunk_offset(get_address_offset(*chunk));
        map.insert(std::pair(chunk_offset, std::set<gaia_offset_t>()));
    }

    // std::cout << "TXN RECORD COUNT = " << log.data()->record_count << std::endl;
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
            std::cout << "WRITING OBJECT SIZE = " << offset_to_ptr(lr->new_offset)->payload_size << std::endl;
            auto chunk = memory_manager->get_chunk_offset(get_address_offset(lr->new_offset));
            ASSERT_INVARIANT(map.count(chunk) > 0, "Can't find chunk.");
            ASSERT_INVARIANT(chunk != c_invalid_chunk_offset, "Invalid chunk offset found.");
            std::cout << "WRITING OFFSET = " << get_address_offset(lr->new_offset) << std::endl;
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
        size_t allocation_size = ((payload_size + gaia::db::memory_manager::c_slot_size - 1) / gaia::db::memory_manager::c_slot_size) * gaia::db::memory_manager::c_slot_size;
        ASSERT_INVARIANT(allocation_size > 0 && allocation_size % gaia::db::memory_manager::c_slot_size == 0, "Invalid allocation size.");
        contiguous_offsets.push_back(end_offset + allocation_size);

        std::cout << "TXN START OFFSET = " << *object_address_offsets.begin() << std::endl;
        std::cout << "TXN END OFFSET = " << end_offset + allocation_size << std::endl;
    }

    ASSERT_INVARIANT(contiguous_offsets.size() % 2 == 0, "We expect a begin and end offset.");

    if (deleted_ids.size() > 0 || contiguous_offsets.size() > 0)
    {
        // Finally make call.
        create_txn_record(commit_ts, record_type_t::txn, contiguous_offsets, deleted_ids);
    }
}

void persistent_log_handler_t::map_commit_ts_to_session_unblock_fd(gaia_txn_id_t commit_ts, int session_unblock_fd)
{
    ASSERT_INVARIANT(session_unblock_fd > 0, "incorrect session unblock fd");
    async_disk_writer->map_commit_ts_to_session_unblock_fd(commit_ts, session_unblock_fd);
    // std::cout << "SAW FD = " << session_unblock_fd << std::endl;
}

void persistent_log_handler_t::validate_flushed_batch()
{
    async_disk_writer->perform_post_completion_maintenence();
}

void persistent_log_handler_t::submit_writes(bool sync)
{
    check_endianness();
    async_disk_writer->handle_submit(current_file->get_file_fd(), sync);
}

void persistent_log_handler_t::create_txn_record(
    gaia_txn_id_t commit_ts,
    record_type_t type,
    std::vector<gaia_offset_t>& contiguous_address_offsets,
    std::vector<gaia_id_t>& deleted_ids)
{
    check_endianness();
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
    persistent_log_file_offset_t start_offset = allocate_log_space(total_log_space_needed);

    // Create header.
    record_header_t header;
    header.crc = c_crc_initial_value;
    header.payload_size = total_log_space_needed;
    header.count = deleted_ids.size();
    header.txn_commit_ts = commit_ts;
    header.record_type = type;

    // std::cout << "TXN RECORD OFFSET = " << start_offset << " AND SIZE = " << total_log_space_needed << std::endl;

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

    // std::cout << "ADD HEADER" << std::endl;
    auto header_ptr = async_disk_writer->copy_into_metadata_buffer(&header, sizeof(record_header_t), current_file->get_file_fd());

    // Update the first iovec entry with the header information.
    writes_to_submit.at(0).iov_base = header_ptr;
    writes_to_submit.at(0).iov_len = sizeof(record_header_t);

    // Allocate space for deleted writes in helper buffer.
    // std::cout << "ADD DEL IDS" << std::endl;
    if (!deleted_ids.empty())
    {
        auto deleted_id_ptr = async_disk_writer->copy_into_metadata_buffer(deleted_ids.data(), deleted_size, current_file->get_file_fd());
        writes_to_submit.push_back({deleted_id_ptr, deleted_size});
    }

    async_disk_writer->construct_pwritev(writes_to_submit, current_file->get_file_fd(), start_offset, current_file->get_current_offset(), uring_op_t::PWRITEV_TXN);
}

void persistent_log_handler_t::destroy_persistent_log()
{
    // Done with recovery, delete all files.
    for (const auto& file : std::filesystem::directory_iterator(s_wal_dir_path))
    {
        std::filesystem::remove_all(file.path());
    }
}

void persistent_log_handler_t::set_persistent_log_sequence(uint64_t log_seq)
{
    file_num = log_seq + 1;
}

void persistent_log_handler_t::recover_from_persistent_log(
    gaia_txn_id_t& last_checkpointed_commit_ts,
    uint64_t& last_processed_log_seq)
{
    // Only relevant for checkpointing. Recovery doesn't care about the
    // 'last_checkpointed_commit_ts' and will reset this field to zero.
    // We don't persist txn ids across restarts.
    max_decided_commit_ts = last_checkpointed_commit_ts;
    std::cout << "Recovering from persistent log." << std::endl;
    // Scan all files and read log records starting from the highest numbered file.
    std::vector<uint64_t> log_files;
    for (const auto& file : std::filesystem::directory_iterator(s_wal_dir_path))
    {
        ASSERT_INVARIANT(file.is_regular_file(), "Only expecting files in persistent log directory.");
        // The file name is just the log sequence number.
        log_files.push_back(std::stoi(file.path().filename()));
    }

    std::cout << "RECOVERY: Number of logs" << log_files.size() << std::endl;

    std::cout << "RECOVERY: Last processed" << last_processed_log_seq << std::endl;

    // Sort files in ascending order by file name.
    sort(log_files.begin(), log_files.end());

    // Apply txns from file.
    for (auto file_seq : log_files)
    {
        // Ignore already processed files.
        if (file_seq <= last_processed_log_seq)
        {
            std::cout << "Skipping sequence = " << file_seq << std::endl;
            continue;
        }

        auto halt_recovery = write_log_file_to_persistent_store(
            s_wal_dir_path,
            file_seq,
            last_checkpointed_commit_ts,
            recovery_mode_t::finish_on_first_error);

        if (halt_recovery)
        {
            ASSERT_INVARIANT(file_seq == log_files.back(), "We don't expect IO issues in intermediate log files.");
            break;
        }
    }

    if (log_files.size() > 0)
    {
        last_processed_log_seq = log_files.back();
    }

    ASSERT_POSTCONDITION(decision_index.size() == 0, "Failed to process all persistent log records.");

    if (log_files.size() > 0)
    {
        last_processed_log_seq = log_files.at(log_files.size() - 1);
    }
    else
    {
        last_processed_log_seq = 0;
    }

    std::cout << "==== RECOVERY DONE ====" << std::endl;
}

bool persistent_log_handler_t::write_log_file_to_persistent_store(std::string& wal_dir_path, uint64_t file_sequence, gaia_txn_id_t& last_checkpointed_commit_ts, recovery_mode_t recovery_mode)
{
    std::stringstream file_name;
    file_name << wal_dir_path << "/" << file_sequence;
    auto file_fd = open(file_name.str().c_str(), O_RDONLY);
    auto file_close = scope_guard::make_scope_guard([&]() {
        close_fd(file_fd);
    });

    if (file_fd < 0)
    {
        throw_system_error("Unable to open persistent log file.", errno);
    }

    record_iterator_t it;
    map_log_file(&it, file_fd, recovery_mode);
    auto mmap_cleanup = scope_guard::make_scope_guard([&]() {
        unmap_file(&it);
    });

    // First index all records in file and validate record checksums.
    index_records_in_file(&it, last_checkpointed_commit_ts);

    // The decision record (set of decision markers for txns) is be the last record in the log, otherwise this is indicative of a crash mid transaction.
    // The decision marker of a transaction is always written after the txn record; thus in the final log file,
    // ignore all transactions after the last decision record.
    // There can be txns that are logged before this decision record and don't have their decision markers
    // Thus any transaction that is greater than the max decision txn should be ignored.
    // In every decision record, we mark the max commit ts that has been made durable. Simply ignore all txn with
    // commit ts greater than this value.

    // Iterate txn records & only write them if decision for them exists.
    // Decision record is always written after the txn record.
    for (auto decision_it = decision_index.cbegin(); decision_it != decision_index.cend();)
    {
        ASSERT_INVARIANT(txn_index.count(decision_it->first) > 0, "Transaction record should be written before the decision record.");

        auto txn_it = txn_index.find(decision_it->first);

        // Only perform recovery and checkpointing for committed transactions.
        if (decision_it->second == txn_decision_type_t::commit)
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

    // Check that any remaining transactions have commit timestamp greater than the commit ts of the txn that was last written to the persistent store.
    for (auto entry : txn_index)
    {
        ASSERT_INVARIANT(entry.first > last_checkpointed_commit_ts, "Expected to find decision record for txn.");
    }

    ASSERT_POSTCONDITION(decision_index.size() == 0, "Failed to process all persistent log records in log file.");

    return it.halt_recovery;
}

void persistent_log_handler_t::write_log_record_to_persistent_store(read_record_t* record)
{
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

        size_t allocation_size = ((requested_size + gaia::db::memory_manager::c_slot_size - 1) / gaia::db::memory_manager::c_slot_size) * gaia::db::memory_manager::c_slot_size;

        ASSERT_INVARIANT(allocation_size > 0 && allocation_size % gaia::db::memory_manager::c_slot_size == 0, "Invalid allocation size.");

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

void persistent_log_handler_t::index_records_in_file(record_iterator_t* it, gaia_txn_id_t last_checkpointed_commit_ts)
{
    size_t record_size = 0;
    size_t record_count = 1;

    do
    {
        // std::cout << "READING RECORD NUMBER = " << record_count << std::endl;

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

        read_record_t* record = reinterpret_cast<read_record_t*>(current_record_ptr);

        if (record_size != 0 && record->header.record_type == record_type_t::decision)
        {
            // std::cout << "Obtained decision record" << std::endl;

            // Decode decision record.
            auto payload_ptr = current_record_ptr + sizeof(record_header_t);

            // Obtain decisions. Decisions may not be in commit order, so sort and process them.
            for (size_t i = 0; i < record->header.count; i++)
            {
                auto decision_entry = reinterpret_cast<decision_record_entry_t*>(payload_ptr);
                if (decision_entry->txn_commit_ts > last_checkpointed_commit_ts)
                {
                    ASSERT_INVARIANT(txn_index.count(decision_entry->txn_commit_ts) > 0, "Transaction record should be written before the decision record.");
                    decision_index.insert(std::pair(decision_entry->txn_commit_ts, decision_entry->decision));
                }
                payload_ptr += sizeof(decision_record_entry_t);
            }
            record_count++;
        }
        else if (record_size != 0 && record->header.record_type == record_type_t::txn)
        {
            // Skip over records that have already been checkpointed.
            if (record->header.txn_commit_ts <= last_checkpointed_commit_ts)
            {
                record_count++;
                continue;
            }
            // std::cout << "txn index inserted = " << record->header.txn_commit_ts << std::endl;
            txn_index.insert(std::pair(record->header.txn_commit_ts, current_record_ptr));
            record_count++;
        }
        else
        {
            ASSERT_INVARIANT(false, "We don't expect another record type");
        }

    } while (true);
}

size_t persistent_log_handler_t::update_cursor(struct record_iterator_t* it)
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

size_t persistent_log_handler_t::validate_recovered_record_crc(struct record_iterator_t* it)
{
    auto destination = reinterpret_cast<read_record_t*>(it->cursor);
    // std::cout << "RECOVERY: CURSOR = " << it->cursor - it->begin  << " AND RECORD = " << (uint8_t) destination->header.record_type << std::endl;

    if (destination->header.payload_size == 0)
    {
        std::cout << "halt here " << std::endl;
    }
    if (destination->header.crc == 0)
    {
        std::cout << "HEADER CRC zero." << std::endl;
        if (it->recovery_mode == recovery_mode_t::fail_on_error)
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
    // A create a copy of the header to avoid modifying the wal log once it has been written.
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
        if (it->recovery_mode == recovery_mode_t::fail_on_error)
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

void persistent_log_handler_t::map_log_file(struct record_iterator_t* it, int file_fd, recovery_mode_t recovery_mode)
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

bool persistent_log_handler_t::is_remaining_file_empty(uint8_t* start, uint8_t* end)
{
    auto remaining_size = end - start;
    uint8_t zeroblock[remaining_size];
    memset(zeroblock, 0, sizeof zeroblock);
    return memcmp(zeroblock, start, remaining_size) == 0;
}

void persistent_log_handler_t::unmap_file(struct record_iterator_t* it)
{
    munmap(it->mapped, it->map_size);
}

} // namespace db
} // namespace gaia
