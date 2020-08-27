/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine.hpp"
#include "storage_engine_server.hpp"
#include "gaia_hash_map.hpp"
#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"
#include "rdb_wrapper.hpp"
#include "rdb_object_converter.hpp"
#include "rdb_internal.hpp"
#include "system_table_types.hpp"
#include "gaia_db_internal.hpp"
#include "types.hpp"
#include <memory>

using namespace gaia::db; 
using namespace gaia::common;
using namespace rocksdb;

// Todo (msj) Take as input to some options file.
static const std::string data_dir = PERSISTENT_DIRECTORY_PATH;
std::unique_ptr<gaia::db::rdb_internal_t> rdb_wrapper::rdb_internal;

// Todo (msj) Set more granular default options.
rdb_wrapper::rdb_wrapper() {
    rocksdb::WriteOptions write_options{};
    write_options.sync = true;
    rocksdb::TransactionDBOptions transaction_db_options{};
    rdb_internal = std::unique_ptr<gaia::db::rdb_internal_t>(new gaia::db::rdb_internal_t(data_dir, write_options, transaction_db_options));
}

rdb_wrapper::~rdb_wrapper() {
    close();
}

Status rdb_wrapper::open() {
    rocksdb::TransactionDBOptions options{};
    rocksdb::Options initoptions{};
    // Implies 2PC log writes.
    initoptions.allow_2pc = true;
    // Create a new database directory if one doesn't exist.
    initoptions.create_if_missing = true;
    // Size of memtable (4 mb)
    initoptions.write_buffer_size = 1 * 1024 * 1024; 
    initoptions.db_write_buffer_size = 1 * 1024 * 1024;

    // Will function as a trigger for flushing memtables to disk.
    // https://github.com/facebook/rocksdb/issues/4180 Only relevant when we have multiple column families.
    initoptions.max_total_wal_size = 1 * 1024 * 1024;
    // Number of memtables; 
    // The maximum number of write buffers that are built up in memory.
    // So that when 1 write buffers being flushed to storage, new writes can continue to the other
    // write buffer.
    initoptions.max_write_buffer_number = 1;

    // The minimum number of write buffers that will be merged together
    // before writing to storage.  If set to 1, then
    // all write buffers are flushed to L0 as individual files.
    initoptions.min_write_buffer_number_to_merge = 1;

    // Any IO error during WAL replay is considered as data corruption.
    // This option assumes clean server shutdown.
    // A crash during a WAL write may lead to the database not getting opened. (see https://github.com/cockroachdb/pebble/issues/453)
    // Currently in place for development purposes. 
    // The default option 'kPointInTimeRecovery' will stop the WAL playback on discovering WAL inconsistency
    // without notifying the caller.
    initoptions.wal_recovery_mode = WALRecoveryMode::kAbsoluteConsistency;

    Status s = rdb_internal->open_txn_db(initoptions, options);

    // RocksDB throws an IOError when trying to open (recover) twice on the same directory 
    // while a process is already up. 
    // The same error is also seen when reopening the db after a large volume of deletes
    // See https://github.com/facebook/rocksdb/issues/4421
    int open_db_attempt_count = 0;
    while (s.code() == c_rocksdb_io_error_code && open_db_attempt_count < c_max_open_db_attempt_count) { 
        open_db_attempt_count++;
        if (rdb_internal->is_db_open()) {
            rdb_internal->close();
            s = rdb_internal->open_txn_db(initoptions, options);
        } else {
            s = rdb_internal->open_txn_db(initoptions, options);
        }
    }
    return s;
}

Status rdb_wrapper::close() {        
    return rdb_internal->close();
}

void rdb_wrapper::commit_tx(rocksdb::Transaction* trx) {
    rocksdb::Status status = rdb_internal->commit_txn(trx);
    
    // Ideally, this should always go through as RocksDB validation is switched off.
    // For now, abort if commit fails.
    if (!status.ok()) {
        abort();
    }
}

rocksdb::Transaction* rdb_wrapper::begin_tx(gaia_xid_t transaction_id) {
    rocksdb::WriteOptions write_options{};
    rocksdb::TransactionOptions txn_options{};
    return rdb_internal->begin_txn(write_options, txn_options, transaction_id);
}

rocksdb::Status rdb_wrapper::rollback_tx(rocksdb::Transaction* trx) {
    return rdb_internal->rollback(trx);
}

Status rdb_wrapper::prepare_tx(rocksdb::Transaction* trx) {
    auto s_log = se_base::get_txn_log();
    // The key_count variable represents the number of puts + deletes.
    size_t key_count = 0;
    for (auto i = 0; i < s_log->count; i++) {
        auto log_record = s_log->log_records + i;
        if (log_record->operation == gaia_operation_t::remove) {
            // Encode key to be deleted.
            string_writer key; 
            key.write_uint64(log_record->deleted_id);
            trx->Delete(key.to_slice());
            key_count++;
        } else {
            string_writer key; 
            string_writer value;
            void* gaia_object = se_base::offset_to_ptr(log_record->new_object, server::s_data);

            if (!gaia_object) {
                // Object was deleted in current transaction.
                continue;
            }
            encode_object(static_cast<object*>(gaia_object), &key, &value);
            // Gaia objects encoded as key-value slices shouldn't be empty.
            assert(key.get_current_position() != 0 && value.get_current_position() != 0);
            trx->Put(key.to_slice(), value.to_slice());
            key_count++;
        } 
    }
    // Ensure that keys were inserted into the RocksDB transaction object.
    assert(key_count == s_log->count);
    return rdb_internal->prepare_txn(trx);
}

/**
 * This API will read the entire LSM in sorted order and construct 
 * gaia_objects using the create API.
 * Additionally, this method will recover the max gaia_id/transaction_id's seen by previous 
 * incarnations of the database. 
 * 
 * Todo (msj) The current implementation has an issue where deleted gaia_ids may get recycled post 
 * recovery. Both the last seen gaia_id & transaction_id need to be
 * persisted to the RocksDB manifest. https://github.com/facebook/rocksdb/wiki/MANIFEST
 * 
 * Note that, for now we skip validating the existence of object references on recovery, 
 * since these aren't validated during object creation either. 
 */
void rdb_wrapper::recover() {
    auto it = std::unique_ptr<rocksdb::Iterator>(rdb_internal->get_iterator());
    gaia_id_t max_id = 0;
    int count = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        auto id = decode_object(it->key(), it->value());
        if (id > max_id && id < c_system_table_reserved_range_start) {
            max_id = id;
        }
        count++;
    }    
    // Check for any errors found during the scan
    assert(it->status().ok());
    server::s_data->next_id = max_id;

    cout << "Recovered count " << count << endl << flush;
    cout << "Recovered max ID " << max_id << endl << flush;

    cout << "Set max ID to " << server::s_data->next_id << endl << flush;

    cout << "[Server] Row ID count on recovery " << server::s_data->row_id_count << endl << flush;
}

void rdb_wrapper::destroy_db() {
    rdb_internal->destroy_db();
}

void rdb_wrapper::create_object_on_recovery(
    gaia_id_t id,
    gaia_type_t type,
    uint64_t num_refs,
    uint64_t data_size,
    const void* data) {
    hash_node* hash_node = gaia_hash_map::insert(server::s_data, server::s_shared_offsets, id);
    hash_node->row_id = server::allocate_row_id(server::s_shared_offsets, server::s_data);
    server::allocate_object(hash_node->row_id, data_size + sizeof(object), server::s_shared_offsets, server::s_data);
    object* obj_ptr = se_base::locator_to_ptr(server::s_shared_offsets, server::s_data, hash_node->row_id);
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->num_references = num_refs;
    obj_ptr->payload_size = data_size;
    memcpy(obj_ptr->payload, data, data_size);
}
