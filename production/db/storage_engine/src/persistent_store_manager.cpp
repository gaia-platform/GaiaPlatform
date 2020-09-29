/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine.hpp"
#include "storage_engine_server.hpp"
#include "gaia_hash_map.hpp"
#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"
#include "persistent_store_manager.hpp"
#include "rdb_object_converter.hpp"
#include "rdb_internal.hpp"
#include "system_table_types.hpp"
#include "gaia_db_internal.hpp"
#include "gaia_record.hpp"
#include <memory>

using namespace gaia::db;
using namespace gaia::common;
using namespace rocksdb;

// Todo (Mihir) Take as input to some options file. https://gaiaplatform.atlassian.net/browse/GAIAPLAT-323
static const std::string data_dir = PERSISTENT_DIRECTORY_PATH;
std::unique_ptr<gaia::db::rdb_internal_t> persistent_store_manager::rdb_internal;

persistent_store_manager::persistent_store_manager() {
    rocksdb::WriteOptions write_options{};
    write_options.sync = true;
    rocksdb::TransactionDBOptions transaction_db_options{};
    rdb_internal.reset(new gaia::db::rdb_internal_t(data_dir, write_options, transaction_db_options));
}

persistent_store_manager::~persistent_store_manager() {
    close();
}

void persistent_store_manager::open() {
    rocksdb::TransactionDBOptions options{};
    rocksdb::Options init_options{};
    // Implies 2PC log writes.
    init_options.allow_2pc = true;
    // Use fsync instead of fdatasync.
    init_options.use_fsync = true;
    // Create a new database directory if one doesn't exist.
    init_options.create_if_missing = true;
    // Size of memtable (4 mb)
    init_options.write_buffer_size = 4 * 1024 * 1024;
    init_options.db_write_buffer_size = 4 * 1024 * 1024;

    // Will function as a trigger for flushing memtables to disk.
    // https://github.com/facebook/rocksdb/issues/4180 Only relevant when we have multiple column families.
    init_options.max_total_wal_size = 4 * 1024 * 1024;
    // Number of memtables;
    // The maximum number of write buffers that are built up in memory.
    // So that when 1 write buffers being flushed to storage, new writes can continue to the other
    // write buffer.
    init_options.max_write_buffer_number = 2;

    // The minimum number of write buffers that will be merged together
    // before writing to storage.  If set to 1, then
    // all write buffers are flushed to L0 as individual files.
    init_options.min_write_buffer_number_to_merge = 1;

    // Any IO error during WAL replay is considered as data corruption.
    // This option assumes clean server shutdown.
    // A crash during a WAL write may lead to the database not getting opened. (see https://github.com/cockroachdb/pebble/issues/453)
    // Currently in place for development purposes.
    // The default option 'kPointInTimeRecovery' will stop the WAL playback on discovering WAL inconsistency
    // without notifying the caller.
    // Todo(Mihir) Update to 'kPointInTimeRecovery' after https://gaiaplatform.atlassian.net/browse/GAIAPLAT-321
    init_options.wal_recovery_mode = WALRecoveryMode::kAbsoluteConsistency;

    rdb_internal->open_txn_db(init_options, options);
}

void persistent_store_manager::close() {
    rdb_internal->close();
}

void persistent_store_manager::append_wal_commit_marker(std::string& txn_name) {
    rdb_internal->commit(txn_name);
}

std::string persistent_store_manager::begin_txn(gaia_xid_t transaction_id) {
    rocksdb::WriteOptions write_options{};
    rocksdb::TransactionOptions txn_options{};
    return rdb_internal->begin_txn(write_options, txn_options, transaction_id);
}

void persistent_store_manager::append_wal_rollback_marker(std::string& txn_name) {
    rdb_internal->rollback(txn_name);
}

void persistent_store_manager::prepare_wal_for_write(std::string& txn_name) {
    auto s_log = se_base::get_txn_log();
    assert(s_log);
    // The key_count variable represents the number of puts + deletes.
    size_t key_count = 0;
    // Obtain RocksDB transaction object.
    auto txn = rdb_internal->get_transaction_by_name(txn_name);
    for (size_t i = 0; i < s_log->count; i++) {
        auto lr = s_log->log_records + i;
        if (lr->operation == gaia_operation_t::remove) {
            // Encode key to be deleted.
            string_writer key;
            key.write_uint64(lr->deleted_id);
            txn->Delete(key.to_slice());
            key_count++;
        } else {
            string_writer key;
            string_writer value;
            void* gaia_object = lr->new_object ? (server::s_data->objects + lr->new_object) : nullptr;
            if (!gaia_object) {
                // Object was deleted in current transaction.
                continue;
            }
            encode_object(static_cast<gaia_se_object_t*>(gaia_object), &key, &value);
            // Gaia objects encoded as key-value slices shouldn't be empty.
            assert(key.get_current_position() != 0 && value.get_current_position() != 0);
            txn->Put(key.to_slice(), value.to_slice());
            key_count++;
        }
    }
    // Ensure that keys were inserted into the RocksDB transaction object.
    assert(key_count == s_log->count);
    rdb_internal->prepare_wal_for_write(txn);
}

/**
 * This API will read the entire LSM in sorted order and construct
 * gaia_objects using the create API.
 * Additionally, this method will recover the max gaia_id/transaction_id's seen by previous
 * incarnations of the database.
 *
 * Todo (Mihir) The current implementation has an issue where deleted gaia_ids may get recycled post
 * recovery. Both the last seen gaia_id & transaction_id need to be
 * persisted to the RocksDB manifest. https://github.com/facebook/rocksdb/wiki/MANIFEST
 *
 * Note that, for now we skip validating the existence of object references on recovery,
 * since these aren't validated during object creation either.
 */
void persistent_store_manager::recover() {
    auto it = std::unique_ptr<rocksdb::Iterator>(rdb_internal->get_iterator());
    gaia_id_t max_id = 0;
    size_t count = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        auto id = decode_object(it->key(), it->value());
        if (id > max_id && id < c_system_table_reserved_range_start) {
            max_id = id;
        }
        count++;
    }
    // Check for any errors found during the scan
    rdb_internal->handle_rdb_error(it->status());
    server::s_data->next_id = max_id;
}

void persistent_store_manager::destroy_persistent_store() {
    rdb_internal->destroy_persistent_store();
}

void persistent_store_manager::create_object_on_recovery(
    gaia_id_t id,
    gaia_type_t type,
    size_t num_refs,
    size_t data_size,
    const void* data) {
    hash_node* hash_node = gaia_hash_map::insert(server::s_data, server::s_shared_offsets, id);
    hash_node->row_id = se_base::allocate_row_id(server::s_shared_offsets, server::s_data, true);
    se_base::allocate_object(hash_node->row_id, data_size + sizeof(gaia_se_object_t), server::s_shared_offsets, server::s_data, true);
    gaia_se_object_t* obj_ptr = server::locator_to_ptr(server::s_shared_offsets, server::s_data, hash_node->row_id);
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->num_references = num_refs;
    obj_ptr->payload_size = data_size;
    memcpy(obj_ptr->payload, data, data_size);
}
