/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction_db.h"

// This file provides gaia specific functionality to 
// 1) commit to RocksDB LSM on write and 
// 2) Read from LSM during recovery 
// Above features are built using the simple RocksDB internal library (rdb_internal.hpp)
// This file will be called by the storage engine and doesn't expose any RocksDB internals
namespace gaia 
{
namespace db 
{

    class rdb_internal;

    class rdb_wrapper 
    {
        private:
            rdb_internal* rdb_internal;

        public:
            rdb_wrapper();
            ~rdb_wrapper();

            /**
             * Open rocksdb with the correct options.
             * Todo(Mihir) Set tuning options https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide
             */
            rocksdb::Status open();

            /**
             * Close the database and delete the database object.
             * This call cannot be reversed.
             */
            rocksdb::Status close();

            /** 
             * Iterate over all elements in the LSM and call storage engine create apis 
             * for every key/value pair obtained (after deduping keys).
             */
            void recover();

            rocksdb::Transaction* begin_tx(gaia_xid_t transaction_id);

            rocksdb::Status prepare_tx(gaia_xid_t transaction_id, rocksdb::Transaction* trx);

            void commit_tx(gaia_xid_t transaction_id, rocksdb::Transaction* trx);

            void destroy();

    };
}

}
