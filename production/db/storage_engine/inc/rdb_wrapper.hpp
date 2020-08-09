/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

using namespace rocksdb;

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
            Status open();

            /**
             * Close the database and delete the database object.
             * This call cannot be reversed.
             */
            Status close();

            /** 
             * Iterate over all elements in the LSM and call storage engine create apis 
             * for every key/value pair obtained (after deduping keys).
             */
            void recover();

            Status prepare_tx(gaia_xid_t transaction_id);

            void commit_tx(gaia_xid_t transaction_id);

            void commit_tx(int64_t trid);

            void destroy();

    };
}

}
