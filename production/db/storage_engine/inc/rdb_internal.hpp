/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "rocksdb/utilities/transaction_db.h"

#include "db_types.hpp"

// Simple library over RocksDB APIs.
namespace gaia
{
namespace db
{

class rdb_internal_t
{
public:
    rdb_internal_t(std::string dir, rocksdb::WriteOptions write_opts, rocksdb::TransactionDBOptions txn_opts)
        : m_txn_db(nullptr), m_data_dir(dir), m_write_options(write_opts), m_txn_options(txn_opts)
    {
    }

    ~rdb_internal_t()
    {
        close();
    }

    void open_txn_db(rocksdb::Options& init_options, rocksdb::TransactionDBOptions& opts);

    std::string begin_txn(rocksdb::WriteOptions& options, const rocksdb::TransactionOptions& txn_opts, gaia_txn_id_t txn_id);

    void prepare_wal_for_write(rocksdb::Transaction* txn);

    void rollback(std::string& txn_name);

    void commit(std::string& txn_name);

    void close();

    rocksdb::Iterator* get_iterator();

    void destroy_persistent_store();

    bool is_db_open();

    rocksdb::Transaction* get_txn_by_name(std::string& txn_name);

    void handle_rdb_error(rocksdb::Status status);

private:
    std::unique_ptr<rocksdb::TransactionDB> m_txn_db;
    std::string m_data_dir;
    rocksdb::WriteOptions m_write_options;
    rocksdb::TransactionDBOptions m_txn_options;
};

} // namespace db
} // namespace gaia
