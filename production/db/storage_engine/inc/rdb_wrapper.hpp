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

    class rdb_wrapper 
    {
        private:
            static std::unique_ptr<gaia::db::rdb_internal> rdb_internal;

        public:
            rdb_wrapper();

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
             * Prepare will serialize the transaction to the log. w
             * Similarly, rollback will append a rollback marker to the log.
             */ 
            rocksdb::Status prepare_tx(rocksdb::Transaction* trx);

            /** 
             * This method will append a commit marker with the appropriate
             * transaction_id to the log.
             */
            void commit_tx(rocksdb::Transaction* trx);

            /**
             * Similarly, rollback will append a rollback marker to the log. 
             */
            rocksdb::Status rollback_tx(rocksdb::Transaction* trx);

    };
}

}
