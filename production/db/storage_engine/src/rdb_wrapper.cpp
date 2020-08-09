/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine.hpp"
#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"
#include "rdb_wrapper.hpp"
#include "rdb_object_converter.hpp"
#include "rdb_internal.hpp"

using namespace gaia::db; 
using namespace rocksdb;

// Todo (msj) Take as input to some options file.
static const std::string data_dir = "/tmp/db";

// Todo (msj) Set more granular default options.
rdb_wrapper::rdb_wrapper() {
    rocksdb::WriteOptions writeOptions{};
    writeOptions.sync = true;
    rocksdb::TransactionDBOptions transaction_db_options{};
    rdb_internal = new gaia::db::rdb_internal(data_dir, writeOptions, transaction_db_options);
}

rdb_wrapper::~rdb_wrapper() {
    delete rdb_internal;
}

Status rdb_wrapper::open() {
    rocksdb::TransactionDBOptions options{};
    rocksdb::Options initoptions{};
    // Every write to the log will require an fsync.
    // Note that RocksDB will perform 1 fsync for a 'group' of parallel transactions.
    initoptions.use_fsync = true;
    // With the current implementation (unoptimized) each transaction will write to the log twice
    // in the commit path; once to prepare & once to append the commit decision to the log.
    initoptions.allow_2pc = true;
    // Create a new database directory if one doesn't exist.
    initoptions.create_if_missing = true;
    return rdb_internal->open_txn_db(initoptions, options);
}

Status rdb_wrapper::close() {        
    return rdb_internal->close();
}

/**
 * This API will delete the persistent database directory.
 * Only used for testing purposes.
 */
void rdb_wrapper::destroy() {        
    rdb_internal->destroy();
}

void rdb_wrapper::commit_tx(int64_t trid) {
    rocksdb::Status status = rdb_internal->commit_txn(trid);
    
    // Ideally, this should always go through as RocksDB validation is switched off.
    // For now, abort if commit fails.

    // Before calling rollback, look at the rollback API impl.
    if (!status.ok()) {
        abort();
    }
}

Status rdb_wrapper::prepare_tx(gaia_xid_t transaction_id) {
    rocksdb::WriteOptions writeOptions{};
    rocksdb::TransactionOptions txnOptions{};
    auto s_log = gaia_mem_base::get_log();
    
    rocksdb::Transaction* trx = rdb_internal->begin_txn(writeOptions, txnOptions, gaia_mem_base::get_trid());

    //set of deleted keys
    set<int64_t> deleted_id;

    for (auto i = 0; i < s_log->count; i++) {

        auto lr = s_log->log_records + i;

        if (lr->operation == GaiaOperation::remove) {
            // Encode key to be deleted.
            // Op1: Reset later
            // Op2: Log key to be deleted.
            string_writer key; 
            // Create key.
            key.write_uint64(lr->fbb_type);
            key.write_uint64(lr->gaia_id);
            trx->Delete(key.to_slice());
        } else {
            string_writer key; 
            string_writer value;
            void* gaia_object = gaia_mem_base::offset_to_ptr(lr->row_id);

            if (!gaia_object) {
                // object was deleted in current tx
                continue;
            }

            rdb_object_converter_util::encode_object(static_cast<object*>(gaia_object), &key, &value);

            // Gaia objects encoded as key-value slices shouldn't be empty.
            assert(key.get_current_position() != 0 && value.get_current_position() != 0);

            trx->Put(key.to_slice(), value.to_slice());
        } 
       
    }

    return rdb_internal->prepare_txn(transaction_id);
}

/**
 * This API will read the entire LSM in sorted order and construct 
 * gaia_objects using the create API.
 * Population the transactional log will be skipped. 
 * Additionally, this method will recover the max gaia_id seen by previous 
 * incarnations of the database. 
 * 
 * Note that, for now we skip validating the existence of object references on recovery, 
 * since these aren't checked during object creation either. 
 */
void rdb_wrapper::read_on_recovery() {
    rocksdb::Iterator* it = rdb_internal->get_iterator();
    uint64_t max_id = gaia_mem_base::get_current_id();
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        rdb_object_converter_util::decode_object(it->key(), it->value(), &max_id);
    }

    gaia_mem_base::set_id(max_id + 1);

    delete it; 
}
