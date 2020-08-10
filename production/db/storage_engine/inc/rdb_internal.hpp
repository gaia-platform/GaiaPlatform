/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction_db.h"

// Simple library over RocksDB APIs.
namespace gaia 
{
namespace db 
{
    
class rdb_internal
{
    private:
        rocksdb::TransactionDB* m_txn_db;
        std::map<int64_t, rocksdb::Transaction*> transaction_map;

        std::string data_dir;
        rocksdb::WriteOptions writeOptions;
        rocksdb::TransactionDBOptions txnOptions;

    public:
        rdb_internal(std::string dir, rocksdb::WriteOptions writeOpts, rocksdb::TransactionDBOptions txnOpts) {
            m_txn_db = nullptr;
            data_dir = dir;
            writeOptions = writeOpts;
            txnOptions = txnOpts;
        }
        
        rocksdb::Status open_txn_db(rocksdb::Options& initOptions, rocksdb::TransactionDBOptions& opts) {
            return rocksdb::TransactionDB::Open(initOptions, opts, data_dir, &m_txn_db);
        }

        rocksdb::Transaction* begin_txn(rocksdb::WriteOptions& options, const rocksdb::TransactionOptions& txnOpts, std::int64_t trid) {
            rocksdb::Transaction* trx = m_txn_db->BeginTransaction(options, txnOpts);
            trx->SetName(std::to_string(trid));
            transaction_map.insert(std::make_pair(trid, trx));
            return trx;
        }

        rocksdb::Status prepare_txn(std::int64_t trid) {
            std::map<int64_t, rocksdb::Transaction*>::iterator it = transaction_map.find(trid);
            if (it != transaction_map.end()) {
                return it->second->Prepare();
            }
        }

        rocksdb::Status commit_txn(std::int64_t trid) {
            std::map<int64_t, rocksdb::Transaction*>::iterator it = transaction_map.find(trid);
            if (it != transaction_map.end()) {
                rocksdb::Status s = it->second->Commit();
                if (s.ok()) {
                    transaction_map.erase(it);
                }
                return s;
            }
        }
        
        rocksdb::Status close() {
            assert(m_txn_db);
        
            rocksdb::Status status = m_txn_db->Close();
            delete m_txn_db;
            m_txn_db = nullptr;

            return status;
        }

        rocksdb::Status write(rocksdb::WriteBatch& batch) {
            return m_txn_db->Write(writeOptions, &batch);
        }

        rocksdb::Status get(rocksdb::Slice& key, std::string* value) {
            return m_txn_db->Get(rocksdb::ReadOptions(), key, value);
        }

        rocksdb::Iterator* get_iterator() {
            return m_txn_db->NewIterator(rocksdb::ReadOptions());
        }

        void destroy() {
            rocksdb::Options opts{};
            rocksdb::DestroyDB(data_dir, opts);
        }

    };
}

}
