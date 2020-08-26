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
    
class rdb_internal_t
{
    private:
        std::unique_ptr<rocksdb::TransactionDB> m_txn_db;

        std::string m_data_dir;
        rocksdb::WriteOptions m_write_options;
        rocksdb::TransactionDBOptions m_txn_options;

    public:
        rdb_internal_t(std::string dir, rocksdb::WriteOptions write_opts, rocksdb::TransactionDBOptions txn_opts) {
            m_txn_db = nullptr;
            m_data_dir = dir;
            m_write_options = write_opts;
            m_txn_options = txn_opts;
        }

        ~rdb_internal_t() {
            close();
        }
        
        rocksdb::Status open_txn_db(rocksdb::Options& init_options, rocksdb::TransactionDBOptions& opts) {
            rocksdb::TransactionDB* txn_db;
            rocksdb::Status s = rocksdb::TransactionDB::Open(init_options, opts, m_data_dir, &txn_db);
            m_txn_db = std::unique_ptr<rocksdb::TransactionDB>(txn_db);
            return s;
        }

        rocksdb::Transaction* begin_txn(rocksdb::WriteOptions& options, const rocksdb::TransactionOptions& txn_opts, gaia_xid_t txn_id) {            
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
            std::stringstream rdb_transaction_name;
            rdb_transaction_name << txn_id << "." << nanoseconds.count();

            rocksdb::Transaction* txn = m_txn_db->BeginTransaction(options, txn_opts);
            rocksdb::Status s = txn->SetName(rdb_transaction_name.str());
            if (!s.ok()) {
                assert(false);
            }
            return txn;
        }

        rocksdb::Status prepare_txn(rocksdb::Transaction* txn) {
            return txn->Prepare();
        }

        rocksdb::Status rollback(rocksdb::Transaction* txn) {
            return txn->Rollback();
        }

        rocksdb::Status commit_txn(rocksdb::Transaction* txn) {
            return txn->Commit();
        }
        
        rocksdb::Status close() {        
            if(m_txn_db.get()) {
                m_txn_db->Close();
            }
        }

        rocksdb::Iterator* get_iterator() {
            return m_txn_db->NewIterator(rocksdb::ReadOptions());
        }

        void destroy_db() {
            rocksdb::DestroyDB(m_data_dir, rocksdb::Options{}); 
        }

        bool is_db_open() {
            return m_txn_db.get();
        }

    };
}

}
