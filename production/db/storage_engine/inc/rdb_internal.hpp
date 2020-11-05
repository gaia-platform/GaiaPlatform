/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sstream>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction_db.h"
#include "rocksdb/write_batch.h"

#include "persistent_store_error.hpp"
#include "retail_assert.hpp"

// Simple library over RocksDB APIs.
namespace gaia
{
namespace db
{
class rdb_internal_t
{
public:
    rdb_internal_t(std::string dir, rocksdb::WriteOptions write_opts, rocksdb::TransactionDBOptions txn_opts)
    {
        m_txn_db = nullptr;
        m_data_dir = dir;
        m_write_options = write_opts;
        m_txn_options = txn_opts;
    }

    ~rdb_internal_t()
    {
        close();
    }

    void open_txn_db(rocksdb::Options& init_options, rocksdb::TransactionDBOptions& opts)
    {
        // RocksDB throws an IOError (Lock on persistent dir) when trying to open (recover) twice on the same directory
        // while a process is already up.
        // The same error is also seen when reopening the db after a large volume of deletes
        // See https://github.com/facebook/rocksdb/issues/4421
        rocksdb::TransactionDB* txn_db;
        rocksdb::Status s;
        s = rocksdb::TransactionDB::Open(init_options, opts, m_data_dir, &txn_db);
        if (s.ok())
        {
            m_txn_db.reset(txn_db);
        }
        else
        {
            handle_rdb_error(s);
        }
        retail_assert(m_txn_db != nullptr, "RocksDB database is not initialized.");
    }

    std::string begin_txn(rocksdb::WriteOptions& options, const rocksdb::TransactionOptions& txn_opts, gaia_txn_id_t txn_id)
    {
        // RocksDB supplies its own transaction id but expects a unique transaction name.
        // We map gaia_txn_id to a RocksDB transaction name. Transaction id isn't
        // persisted across server reboots currently so this is a temporary fix till we have
        // a solution in place. Regardless, RocksDB transactions will go away in Persistence V2.
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
        std::stringstream rdb_txn_name;
        rdb_txn_name << txn_id << "." << nanoseconds.count();
        retail_assert(m_txn_db != nullptr, "RocksDB database is not initialized.");
        rocksdb::Transaction* txn = m_txn_db->BeginTransaction(options, txn_opts);
        rocksdb::Status s = txn->SetName(rdb_txn_name.str());
        handle_rdb_error(s);
        return rdb_txn_name.str();
    }

    void prepare_wal_for_write(rocksdb::Transaction* txn)
    {
        auto s = txn->Prepare();
        handle_rdb_error(s);
    }

    void rollback(std::string& txn_name)
    {
        auto txn = get_txn_by_name(txn_name);
        auto s = txn->Rollback();
        handle_rdb_error(s);
    }

    void commit(std::string& txn_name)
    {
        auto txn = get_txn_by_name(txn_name);
        auto s = txn->Commit();
        handle_rdb_error(s);
    }

    void close()
    {
        if (m_txn_db)
        {
            // Although we could have best effort close(), lets
            // handle any returned failure.
            auto s = m_txn_db->Close();
            handle_rdb_error(s);
        }
    }

    rocksdb::Iterator* get_iterator()
    {
        retail_assert(m_txn_db != nullptr, "RocksDB database is not initialized.");
        return m_txn_db->NewIterator(rocksdb::ReadOptions());
    }

    void destroy_persistent_store()
    {
        rocksdb::DestroyDB(m_data_dir, rocksdb::Options{});
    }

    bool is_db_open()
    {
        return bool(m_txn_db);
    }

    rocksdb::Transaction* get_txn_by_name(std::string& txn_name)
    {
        retail_assert(m_txn_db != nullptr, "RocksDB database is not initialized.");
        return m_txn_db->GetTransactionByName(txn_name);
    }

    void handle_rdb_error(rocksdb::Status status)
    {
        // Todo (Mihir) Additionally log status information.
        if (!status.ok())
        {
            throw persistent_store_error(status.getState(), status.code());
        }
    }

private:
    std::unique_ptr<rocksdb::TransactionDB> m_txn_db;

    std::string m_data_dir;
    rocksdb::WriteOptions m_write_options;
    rocksdb::TransactionDBOptions m_txn_options;
};

} // namespace db
} // namespace gaia
