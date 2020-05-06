/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// This file provides gaia specific functionality to 
// 1) commit to RocksDB LSM on write and 
// 2) Read from LSM during recovery 
// Above features are built using the simple RocksDB internal library (rdb_internal.hpp)
// This file will be called by the storage engine and doesn't expose any RocksDB internals
namespace gaia 
{
namespace db 
{

    struct rdb_status {
        int code;
        std::string msg;
    };
    
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
            rdb_status open();

            /**
             * Close the database and delete the database object.
             * This call cannot be reversed.
             */
            rdb_status close();

            /** 
             * Iterate over all elements in the LSM and call storage engine create apis 
             * for every key/value pair obtained (after deduping keys).
             */
            rdb_status read_on_recovery();

            /**
            * Notes
            * The current implementation will abort on LSM write errors; this is safe since the log will be populated 
            * before writing to LSM, and state can be correctly recovered from the log on startup. 
            * 
            * The alternative is retrying write batches; rocksdb internally assigns sequence numbers per batch and the retried
            * batch will obtain a new sequence number thereby possibly overwriting a newer update. We'd need to encode our
            * own sequence numbers for each batch and override rocksdb duplicate key compaction behavior to respect the sequencing 
            * information we provide.
            * 
            * For now, we will sigabrt (pending further testing to guage rocksdb write stability)
            * 
            * Input: Map of object locators and type per object (node or edge)
            * Type information is required for serialization purposes.
            */    
            void write_on_commit(std::map<int64_t, int8_t> row_ids_with_type);

    };
}

}
