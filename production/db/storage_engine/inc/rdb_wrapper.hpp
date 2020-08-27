/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction_db.h"
#include "rdb_internal.hpp"
#include <memory>

// This file provides gaia specific functionality to persist writes to & read from 
// RocksDB during recovery.
// This file will be called by the storage engine & leverages the simple RocksDB internal library (rdb_internal.hpp)
namespace gaia 
{
namespace db 
{

constexpr uint64_t c_rocksdb_io_error_code = 5;
constexpr uint64_t c_max_open_db_attempt_count = 10;

class rdb_wrapper 
{
    private:
    static std::unique_ptr<gaia::db::rdb_internal_t> rdb_internal;

    public:
    rdb_wrapper();
    ~rdb_wrapper();

    /**
     * Open rocksdb with the correct options.
     */
    rocksdb::Status open();

    /**
     * Close the database.
     */
    rocksdb::Status close();

    /** 
     * Iterate over all elements in the LSM and call SE create API 
     * for every key/value pair obtained (after deduping keys).
     */
    void recover();

    rocksdb::Transaction* begin_tx(gaia_xid_t transaction_id);

    /**
     * Prepare will serialize the transaction to the log. 
     */ 
    rocksdb::Status prepare_tx(rocksdb::Transaction* trx);

    /** 
     * This method will append a commit marker with the appropriate
     * transaction_id to the log, and will additionally insert entries
     * into the RocksDB write buffer (which then writes to disk on getting full)
     */
    void commit_tx(rocksdb::Transaction* trx);

    /**
     * Similarly, rollback will append a rollback marker to the log. 
     */
    rocksdb::Status rollback_tx(rocksdb::Transaction* trx);

    /**
     * Destroy the persistent store.
     */
    void destroy_db();

    /**
     * This method is used to create a Gaia object using a decoded key-value pair from RocksDB.
     */
    static void create_object_on_recovery(
        gaia_id_t id,
        gaia_type_t type,
        uint64_t num_refs,
        uint64_t data_size,
        const void* data);

    };
}

}
