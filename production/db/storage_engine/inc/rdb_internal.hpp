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
            if (!options.sync) {
                options.sync = true;
            }
            rocksdb::Transaction* trx = m_txn_db->BeginTransaction(options, txnOpts);
            trx->SetName(std::to_string(trid));
            return trx;
        }

        rocksdb::Status prepare_txn(rocksdb::Transaction* trx) {
            return trx->Prepare();
        }

        rocksdb::Status rollback(rocksdb::Transaction* trx) {
            return trx->Rollback();
        }

        rocksdb::Status commit_txn(rocksdb::Transaction* trx) {
            return trx->Commit();
        }
        
        rocksdb::Status close() {
            assert(m_txn_db);
        
            rocksdb::Status status = m_txn_db->Close();
            delete m_txn_db;
            m_txn_db = nullptr;

            return status;
        }

        rocksdb::Iterator* get_iterator() {
            return m_txn_db->NewIterator(rocksdb::ReadOptions());
        }

    };
}

}
