////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <utility>

#include <rocksdb/utilities/transaction_db.h>

#include "gaia_internal/db/db_types.hpp"

// Simple library over RocksDB APIs.
namespace gaia
{
namespace db
{

class rdb_wrapper_t
{
public:
    rdb_wrapper_t(std::string dir, const rocksdb::WriteOptions& write_opts, rocksdb::TransactionDBOptions txn_opts)
        : m_txn_db(nullptr), m_data_dir(std::move(dir)), m_write_options(write_opts), m_txn_options(std::move(txn_opts))
    {
    }

    ~rdb_wrapper_t()
    {
        close();
    }

    void open_txn_db(const rocksdb::Options& init_options, const rocksdb::TransactionDBOptions& opts);

    std::string begin_txn(const rocksdb::WriteOptions& options, const rocksdb::TransactionOptions& txn_opts, gaia_txn_id_t txn_id);

    void prepare_wal_for_write(rocksdb::Transaction* txn);

    void rollback(const std::string& txn_name);

    void commit(const std::string& txn_name);

    void close();

    rocksdb::Iterator* get_iterator();

    void destroy_persistent_store();

    bool is_db_open();

    rocksdb::Transaction* get_txn_by_name(const std::string& txn_name);

    void handle_rdb_error(rocksdb::Status status);

private:
    std::unique_ptr<rocksdb::TransactionDB> m_txn_db;
    std::string m_data_dir;
    rocksdb::WriteOptions m_write_options;
    rocksdb::TransactionDBOptions m_txn_options;
};

} // namespace db
} // namespace gaia
