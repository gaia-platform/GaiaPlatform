/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/status.h"

// Simple library over RocksDB APIs, and has no knowledge about the storage engine
namespace gaia 
{
namespace db 
{
    
class rdb_internal
{
    private:
        rocksdb::DB* m_rdb;
        std::string data_dir;
        rocksdb::WriteOptions writeOptions;

    public:
        rdb_internal(std::string dir, rocksdb::WriteOptions writeOpts) {
            m_rdb = nullptr;
            data_dir = dir;
            writeOptions = writeOpts;
        }

        rocksdb::Status open(rocksdb::Options& initOptions) {
            return rocksdb::DB::Open(initOptions, data_dir, &m_rdb);
        }

        rocksdb::Status close() {
            assert(m_rdb);
        
            rocksdb::Status status = m_rdb->Close();
            delete m_rdb;
            m_rdb = nullptr;

            return status;
        }

        rocksdb::Status write(rocksdb::WriteBatch& batch) {
            return m_rdb->Write(writeOptions, &batch);
        }

        rocksdb::Status get(rocksdb::Slice& key, std::string* value) {
            return m_rdb->Get(rocksdb::ReadOptions(), key, value);
        }

        rocksdb::Iterator* get_iterator() {
            return m_rdb->NewIterator(rocksdb::ReadOptions());
        }

    };
}

}
