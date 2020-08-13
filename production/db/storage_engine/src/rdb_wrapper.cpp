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

using namespace gaia::db; 
using namespace gaia::common;
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
    initoptions.use_fsync = true;
    // Implies 2PC log writes.
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
        } else {
            string_writer key; 
            string_writer value;
            void* gaia_object = server::offset_to_ptr(lr->row_id);

            if (!gaia_object) {
                // Object was deleted in current transaction.
                continue;
            }

            rdb_object_converter_util::encode_object((object*) gaia_object, &key, &value);

            // Gaia objects encoded as key-value slices shouldn't be empty.
            assert(key.get_current_position() != 0 && value.get_current_position() != 0);

            trx->Put(key.to_slice(), value.to_slice());
        } 
       
    }

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
    uint64_t max_id = se_base::get_current_id();
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        rdb_object_converter_util::decode_object(it->key(), it->value(), &max_id);
    }

    se_base::set_id(max_id + 1);

    delete it; 
}
