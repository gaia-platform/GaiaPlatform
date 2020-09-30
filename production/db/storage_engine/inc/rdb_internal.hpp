/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sstream>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction_db.h"

#include "persistent_store_error.hpp"

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

    void open_txn_db(rocksdb::Options& init_options, rocksdb::TransactionDBOptions& opts) {
        // RocksDB throws an IOError when trying to open (recover) twice on the same directory
        // while a process is already up.
        // The same error is also seen when reopening the db after a large volume of deletes
        // See https://github.com/facebook/rocksdb/issues/4421
        size_t open_db_attempt_count = 0;
        rocksdb::TransactionDB* txn_db;
        rocksdb::Status s;
        while (open_db_attempt_count < c_max_open_db_attempt_count) {
            s = rocksdb::TransactionDB::Open(init_options, opts, m_data_dir, &txn_db);
            open_db_attempt_count++;
            if (s.code() == rocksdb::Status::Code::kIOError) {
                // Try closing RocksDB so we can open on next retry.
                s = txn_db->Close();
                // Handle error on unsuccessful close.
                handle_rdb_error(s);
            } else if (s.ok()) {
                // Only set m_txn_db if opening RocksDB is successful.
                m_txn_db.reset(txn_db);
                break;
            } else {
                // Status code is neither ok, not IOError; handle the error.
                handle_rdb_error(s);
            }
        }
    }

    std::string begin_txn(rocksdb::WriteOptions& options, const rocksdb::TransactionOptions& txn_opts, gaia_xid_t txn_id) {
        // RocksDB supplies its own transaction id but expects a unique transaction name.
        // We map gaia_transaction_id to a RocksDB transaction name. Transaction id isn't
        // persisted across server reboots currently so this is a temporary fix till we have
        // a solution in place. Regardless, RocksDB transactions will go away in Persistence V2.
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
        std::stringstream rdb_transaction_name;
        rdb_transaction_name << txn_id << "." << nanoseconds.count();

        rocksdb::Transaction* txn = m_txn_db->BeginTransaction(options, txn_opts);
        rocksdb::Status s = txn->SetName(rdb_transaction_name.str());
        handle_rdb_error(s);
        return rdb_transaction_name.str();
    }

    void prepare_wal_for_write(rocksdb::Transaction* txn) {
        auto s = txn->Prepare();
        handle_rdb_error(s);
    }

    void rollback(std::string& txn_name) {
        auto txn = get_transaction_by_name(txn_name);
        auto s = txn->Rollback();
        handle_rdb_error(s);
    }

    void commit(std::string& txn_name) {
        auto txn = get_transaction_by_name(txn_name);
        auto s = txn->Commit();
        handle_rdb_error(s);
    }

    void close() {
        if (m_txn_db) {
            // Although we could have best effort close(), lets
            // handle any returned failure.
            auto s = m_txn_db->Close();
            handle_rdb_error(s);
        }
    }

    rocksdb::Iterator* get_iterator() {
        return m_txn_db->NewIterator(rocksdb::ReadOptions());
    }

    void destroy_persistent_store() {
        rocksdb::DestroyDB(m_data_dir, rocksdb::Options{});
    }

    bool is_db_open() {
        return bool(m_txn_db);
    }

    rocksdb::Transaction* get_transaction_by_name(std::string& txn_name) {
        assert(m_txn_db);
        return m_txn_db->GetTransactionByName(txn_name);
    }

    void handle_rdb_error(rocksdb::Status status) {
        // Todo (Mihir) Additionally log status information.
        if (!status.ok()) {
            throw persistent_store_error(status.getState(), status.code());
        }
    }

};
}
}
