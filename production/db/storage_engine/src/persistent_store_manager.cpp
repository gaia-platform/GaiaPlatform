/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "persistent_store_manager.hpp"

#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"

#include "db_types.hpp"
#include "gaia_db_internal.hpp"
#include "gaia_hash_map.hpp"
#include "gaia_se_object.hpp"
#include "rdb_internal.hpp"
#include "rdb_object_converter.hpp"
#include "storage_engine.hpp"
#include "storage_engine_server.hpp"
#include "system_table_types.hpp"

using namespace gaia::db;
using namespace gaia::common;
using namespace rocksdb;

// Todo (Mihir) Take as input to some options file. https://gaiaplatform.atlassian.net/browse/GAIAPLAT-323
static const std::string c_data_dir = PERSISTENT_DIRECTORY_PATH;
std::unique_ptr<gaia::db::rdb_internal_t> persistent_store_manager::rdb_internal;

persistent_store_manager::persistent_store_manager()
{
    rocksdb::WriteOptions write_options{};
    write_options.sync = true;
    rocksdb::TransactionDBOptions transaction_db_options{};
    rdb_internal = make_unique<gaia::db::rdb_internal_t>(c_data_dir, write_options, transaction_db_options);
}

persistent_store_manager::~persistent_store_manager()
{
    close();
}

void persistent_store_manager::open()
{
    rocksdb::TransactionDBOptions options{};
    rocksdb::Options init_options{};

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

    rdb_internal->open_txn_db(init_options, options);
}

void persistent_store_manager::close()
{
    rdb_internal->close();
}

void persistent_store_manager::append_wal_commit_marker(std::string& txn_name)
{
    rdb_internal->commit(txn_name);
}

std::string persistent_store_manager::begin_txn(gaia_txn_id_t txn_id)
{
    rocksdb::WriteOptions write_options{};
    rocksdb::TransactionOptions txn_options{};
    return rdb_internal->begin_txn(write_options, txn_options, txn_id);
}

void persistent_store_manager::append_wal_rollback_marker(std::string& txn_name)
{
    rdb_internal->rollback(txn_name);
}

void persistent_store_manager::prepare_wal_for_write(std::string& txn_name)
{
    auto log = se_base::get_txn_log();
    retail_assert(log);
    // The key_count variable represents the number of puts + deletes.
    size_t key_count = 0;
    // Obtain RocksDB transaction object.
    auto txn = rdb_internal->get_txn_by_name(txn_name);
    for (size_t i = 0; i < log->count; i++)
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
            void* gaia_object = lr->new_offset ? (server::s_data->objects + lr->new_offset) : nullptr;
            if (!gaia_object)
            {
                // Object was deleted in current transaction.
                continue;
            }
            encode_object(static_cast<gaia_se_object_t*>(gaia_object), &key, &value);
            // Gaia objects encoded as key-value slices shouldn't be empty.
            retail_assert(key.get_current_position() != 0 && value.get_current_position() != 0);
            txn->Put(key.to_slice(), value.to_slice());
            key_count++;
        }
    }
    // Ensure that keys were inserted into the RocksDB transaction object.
    retail_assert(key_count == log->count);
    rdb_internal->prepare_wal_for_write(txn);
}

/**
 * This API will read the entire LSM in sorted order and construct
 * gaia_objects using the create API.
 * Additionally, this method will recover the max gaia_id/txn_id's seen by previous
 * incarnations of the database.
 *
 * TODO: (Mihir) The current implementation has an issue where deleted gaia_ids may get recycled post
 * recovery. Both the last seen gaia_id & txn_id need to be
 * persisted to the RocksDB manifest. https://github.com/facebook/rocksdb/wiki/MANIFEST
 *
 * TODO: (Mihir) For now we skip validating the existence of object references on recovery,
 * since these aren't validated during object creation either.
 */
void persistent_store_manager::recover()
{
    // Create a backward iterator to obtain the last key and thus the largest gaia ID.
    gaia_id_t max_id = 0;
    auto iterate_to_last = std::unique_ptr<rocksdb::Iterator>(rdb_internal->get_iterator());
    for (iterate_to_last->SeekToLast(); iterate_to_last->Valid(); iterate_to_last->Prev())
    {
        max_id = decode_key(iterate_to_last->key());
        // Break after one loop.
        break;
    }
    rdb_internal->handle_rdb_error(iterate_to_last->status());

    // Create another forward iterator for recovery.
    auto it = std::unique_ptr<rocksdb::Iterator>(rdb_internal->get_iterator());
    gaia_type_t max_type_id = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        gaia_type_t type_id = decode_object(it->key(), it->value());
        if (type_id > max_type_id && type_id < c_system_table_reserved_range_start) {
            max_type_id = type_id;
        }
    }
    // Check for any errors found during the scan
    rdb_internal->handle_rdb_error(it->status());
    server::s_data->next_object_id = max_id;
    server::s_data->next_type_id = max_type_id;
}

void persistent_store_manager::destroy_persistent_store()
{
    rdb_internal->destroy_persistent_store();
}

void persistent_store_manager::create_object_on_recovery(
    gaia_id_t id,
    gaia_type_t type,
    size_t num_refs,
    size_t data_size,
    const void* data)
{
    server::hash_node* hash_node = gaia_hash_map::insert(server::s_data, server::s_shared_locators, id);
    hash_node->locator = se_base::allocate_locator(server::s_shared_locators, server::s_data, true);
    se_base::allocate_object(hash_node->locator, data_size + sizeof(gaia_se_object_t), server::s_shared_locators, server::s_data, true);
    gaia_se_object_t* obj_ptr = se_base::locator_to_ptr(server::s_shared_locators, server::s_data, hash_node->locator);
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->num_references = num_refs;
    obj_ptr->payload_size = data_size;
    memcpy(obj_ptr->payload, data, data_size);
}
