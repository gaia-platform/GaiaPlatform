/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "persistent_store_manager.hpp"

#include <utility>

#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/write_batch.h>

#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "rdb_internal.hpp"
#include "rdb_object_converter.hpp"

using namespace std;

using namespace gaia::db;
using namespace gaia::db::persistence;
using namespace gaia::common;
using namespace rocksdb;

persistent_store_manager::persistent_store_manager(
    gaia::db::counters_t* counters, std::string data_dir)
    : m_counters(counters), m_data_dir_path(std::move(data_dir))
{
    rocksdb::WriteOptions write_options{};
    write_options.sync = true;
    rocksdb::TransactionDBOptions transaction_db_options{};
    m_rdb_internal = make_unique<gaia::db::rdb_internal_t>(m_data_dir_path, write_options, transaction_db_options);
}

persistent_store_manager::~persistent_store_manager()
{
    close();
}

void persistent_store_manager::open()
{
    rocksdb::TransactionDBOptions options{};
    rocksdb::Options init_options{};
    rocksdb::BlockBasedTableOptions table_options{};

    // Implies 2PC log writes.
    constexpr bool c_allow_2pc = true;
    // Use fsync instead of fdatasync.
    constexpr bool c_use_fsync = true;
    // Create a new database directory if one doesn't exist.
    constexpr bool c_create_if_missing = true;
    // Size of memtable (4MB).
    constexpr size_t c_write_buffer_size = 4 * 1024 * 1024;
    // Size of memtable across all column families.
    constexpr size_t c_db_write_buffer_size = c_write_buffer_size;
    // Will function as a trigger for flushing memtables to disk.
    // https://github.com/facebook/rocksdb/issues/4180 Only relevant when we have multiple column families.
    // Same size as memtable.
    constexpr size_t c_max_total_wal_size = c_write_buffer_size;
    // Number of memtables.
    // We use 2 memtables, so that while one memtable is being flushed to
    // storage, the other memtable can continue accepting writes.
    constexpr size_t c_max_write_buffer_number = 2;
    // The minimum number of memtables that will be merged together before
    // writing to storage.  If set to 1, then all memtables are flushed to L0 as
    // individual files.
    constexpr size_t c_min_write_buffer_number_to_merge = 1;
    // Any IO error during WAL replay is considered as data corruption. This
    // option assumes clean server shutdown. A crash during a WAL write may lead
    // to the database not getting opened. (see
    // https://github.com/cockroachdb/pebble/issues/453).
    // Currently in place for development purposes. The default option
    // 'kPointInTimeRecovery' will stop the WAL playback on discovering WAL
    // inconsistency without notifying the caller.
    // TODO (Mihir): Update to 'kPointInTimeRecovery' after
    // https://gaiaplatform.atlassian.net/browse/GAIAPLAT-321.
    constexpr auto c_wal_recovery_mode = WALRecoveryMode::kAbsoluteConsistency;

    init_options.allow_2pc = c_allow_2pc;
    init_options.use_fsync = c_use_fsync;
    init_options.create_if_missing = c_create_if_missing;
    init_options.write_buffer_size = c_write_buffer_size;
    init_options.db_write_buffer_size = c_db_write_buffer_size;
    init_options.max_total_wal_size = c_max_total_wal_size;
    init_options.max_write_buffer_number = c_max_write_buffer_number;
    init_options.min_write_buffer_number_to_merge = c_min_write_buffer_number_to_merge;
    init_options.wal_recovery_mode = c_wal_recovery_mode;

    // Block cache is where RocksDB caches data in memory for reads, which we don't need at all.
    table_options.no_block_cache = true;
    auto table_factory = NewBlockBasedTableFactory(table_options);
    init_options.table_factory.reset(table_factory);

    m_rdb_internal->open_txn_db(init_options, options);
}

void persistent_store_manager::close()
{
    m_rdb_internal->close();
}

void persistent_store_manager::append_wal_commit_marker(const std::string& txn_name)
{
    m_rdb_internal->commit(txn_name);
}

std::string persistent_store_manager::begin_txn(gaia_txn_id_t txn_id)
{
    rocksdb::WriteOptions write_options{};
    rocksdb::TransactionOptions txn_options{};
    return m_rdb_internal->begin_txn(write_options, txn_options, txn_id);
}

void persistent_store_manager::append_wal_rollback_marker(const std::string& txn_name)
{
    m_rdb_internal->rollback(txn_name);
}

void persistent_store_manager::prepare_wal_for_write(gaia::db::txn_log_t* log, const std::string& txn_name)
{
    ASSERT_PRECONDITION(log, "Transaction log is null!");
    // The key_count variable represents the number of puts + deletes.
    size_t key_count = 0;
    // Obtain RocksDB transaction object.
    auto txn = m_rdb_internal->get_txn_by_name(txn_name);
    for (size_t i = 0; i < log->record_count; i++)
    {
        auto lr = log->log_records + i;
        if (lr->operation == gaia_operation_t::remove)
        {
            // Encode key to be deleted.
            string_writer_t key;
            key.write_uint64(lr->deleted_id);
            txn->Delete(key.to_slice());
            key_count++;
        }
        else
        {
            string_writer_t key;
            string_writer_t value;
            auto obj = offset_to_ptr(lr->new_offset);
            if (!obj)
            {
                // Object was deleted in current transaction.
                continue;
            }
            encode_object(obj, key, value);
            // Gaia objects encoded as key-value slices shouldn't be empty.
            ASSERT_INVARIANT(
                key.get_current_position() != 0 && value.get_current_position() != 0,
                "Failed to encode object.");
            txn->Put(key.to_slice(), value.to_slice());
            key_count++;
        }
    }
    // Ensure that keys were inserted into the RocksDB transaction object.
    ASSERT_POSTCONDITION(key_count == log->record_count, "Count of inserted objects differs from log count!");
    m_rdb_internal->prepare_wal_for_write(txn);
}

/**
 * This API will read the entire LSM in sorted order and construct
 * gaia_objects using the create API.
 * Additionally, this method will recover the max gaia_id and gaia_type seen by previous
 * incarnations of the database.
 *
 * Todo (Mihir) The current implementation has an issue where deleted gaia_ids may get recycled post
 * recovery. Both the last seen gaia_id & txn_id need to be
 * persisted to the RocksDB manifest. https://github.com/facebook/rocksdb/wiki/MANIFEST
 *
 * Todo(Mihir) Note that, for now we skip validating the existence of object references on recovery,
 * since these aren't validated during object creation either.
 */
void persistent_store_manager::recover()
{
    auto it = std::unique_ptr<rocksdb::Iterator>(m_rdb_internal->get_iterator());
    gaia_id_t max_id = 0;
    gaia_type_t max_type_id = 0;

    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        db_object_t* recovered_object = decode_object(it->key(), it->value());
        if (recovered_object->type > max_type_id && recovered_object->type < c_system_table_reserved_range_start)
        {
            max_type_id = recovered_object->type;
        }

        if (recovered_object->id > max_id)
        {
            max_id = recovered_object->id;
        }
    }
    // Check for any errors found during the scan
    m_rdb_internal->handle_rdb_error(it->status());
    m_counters->last_id = max_id;
    m_counters->last_type_id = max_type_id;

    // Ensure that other threads (with appropriate acquire barriers) immediately
    // observe the changed value. (This could be changed to a release barrier.)
    __sync_synchronize();
}

void persistent_store_manager::destroy_persistent_store()
{
    m_rdb_internal->destroy_persistent_store();
}
