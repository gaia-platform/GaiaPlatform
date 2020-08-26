/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine.hpp"
#include "storage_engine_server.hpp"
#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"
#include "rdb_wrapper.hpp"
#include "rdb_object_converter.hpp"
#include "rdb_internal.hpp"
#include "system_table_types.hpp"
#include "gaia_db_internal.hpp"
#include <memory>

using namespace gaia::db; 
using namespace gaia::common;
using namespace rocksdb;

// Todo (msj) Take as input to some options file.
static const std::string data_dir = PERSISTENT_DIRECTORY_PATH;
std::unique_ptr<gaia::db::rdb_internal> rdb_wrapper::rdb_internal;

// Todo (msj) Set more granular default options.
rdb_wrapper::rdb_wrapper() {
    rocksdb::WriteOptions writeOptions{};
    writeOptions.sync = true;
    rocksdb::TransactionDBOptions transaction_db_options{};
    rdb_internal = std::unique_ptr<gaia::db::rdb_internal>(new gaia::db::rdb_internal(data_dir, writeOptions, transaction_db_options));
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

    // IOError due
    // RocksDB error means that you're trying to open RocksDB twice on the same directory. 
    // The second RocksDB open will fail with this error.
    // See https://github.com/google/leveldb/blob/53e280b56866ac4c90a9f5fcfe02ebdfd4a19832/util/env_posix.cc#L466-L468
    // This seems like a known issue: https://github.com/facebook/rocksdb/issues/4421
    int open_db_attempt_count = 0;
    while (s.code() == 5 && open_db_attempt_count < 10) { // Grep string for lock
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
    rocksdb::WriteOptions writeOptions{};
    rocksdb::TransactionOptions txnOptions{};
    return rdb_internal->begin_txn(writeOptions, txnOptions, transaction_id);
}

rocksdb::Status rdb_wrapper::rollback_tx(rocksdb::Transaction* trx) {
    return rdb_internal->rollback(trx);
}

Status rdb_wrapper::prepare_tx(rocksdb::Transaction* trx) {
    auto s_log = se_base::s_log;

    for (auto i = 0; i < s_log->count; i++) {

        auto lr = s_log->log_records + i;

        if (lr->operation == se_base::gaia_operation_t::remove) {
            // Encode key to be deleted.
            string_writer key; 
            key.write_uint64(lr->id);
            trx->Delete(key.to_slice());
            cout << "ID that was deleted: " << lr->id << endl << flush;
        } else {
            string_writer key; 
            string_writer value;
            void* gaia_object = se_base::offset_to_ptr(lr->new_object, server::s_data);

            if (!gaia_object) {
                // Object was deleted in current transaction.
                cout << "Operation that was skipped: operation; id " << lr->operation << ":" << lr->id << endl << flush;
                continue;
            }

            rdb_object_converter_util::encode_object((object*) gaia_object, &key, &value);

            // Gaia objects encoded as key-value slices shouldn't be empty.
            assert(key.get_current_position() != 0 && value.get_current_position() != 0);

            trx->Put(key.to_slice(), value.to_slice());
        } 
       
    }

    // Ensure that keys were inserted into the RocksDB transaction object.
    cout << "Server log count:trx_keys " << s_log->count << ":" << trx->GetNumKeys() << endl << flush;
    assert(trx->GetNumPuts() +  trx->GetNumDeletes() == s_log->count);

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
    rocksdb::Iterator* it = rdb_internal->get_iterator();
    gaia_id_t max_id = 0;
    int count = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        auto id = rdb_object_converter_util::decode_object(it->key(), it->value());
        if (id > max_id && id < c_system_table_reserved_range_start) {
            max_id = id;
        }
        count++;
    }    
    // Check for any errors found during the scan
    assert(it->status().ok());
    server::s_data->next_id = max_id + 1;

    cout << "Recovered count " << count << endl << flush;
    cout << "Recovered max ID " << max_id << endl << flush;

    cout << "Set max ID to " << server::s_data->next_id << endl << flush;

    cout << "[Server] Row ID count on recovery " << server::s_data->row_id_count << endl << flush;

    delete it; 
}

void rdb_wrapper::destroy_db() {
    rdb_internal->destroy_db();
}
