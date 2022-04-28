/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rdb_wrapper.hpp"

#include <sstream>
#include <string>

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/write_batch.h>

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/persistent_store_error.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{

static constexpr char c_message_rocksdb_not_initialized[] = "RocksDB database is not initialized.";

void rdb_wrapper_t::open_txn_db(const rocksdb::Options& init_options, const rocksdb::TransactionDBOptions& opts)
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
    ASSERT_POSTCONDITION(m_txn_db != nullptr, c_message_rocksdb_not_initialized);
}

std::string rdb_wrapper_t::begin_txn(const rocksdb::WriteOptions& options, const rocksdb::TransactionOptions& txn_opts, gaia_txn_id_t txn_id)
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
    ASSERT_PRECONDITION(m_txn_db != nullptr, c_message_rocksdb_not_initialized);
    rocksdb::Transaction* txn = m_txn_db->BeginTransaction(options, txn_opts);
    rocksdb::Status s = txn->SetName(rdb_txn_name.str());
    handle_rdb_error(s);
    return rdb_txn_name.str();
}

void rdb_wrapper_t::prepare_wal_for_write(rocksdb::Transaction* txn)
{
    auto s = txn->Prepare();
    handle_rdb_error(s);
}

void rdb_wrapper_t::rollback(const std::string& txn_name)
{
    // https://github.com/facebook/rocksdb/blob/master/include/rocksdb/utilities/transaction_db.h#L398
    // Caller is responsible for deleting the returned transaction when no longer needed.
    auto txn = std::unique_ptr<rocksdb::Transaction>(get_txn_by_name(txn_name));
    auto s = txn->Rollback();
    handle_rdb_error(s);
}

void rdb_wrapper_t::commit(const std::string& txn_name)
{
    // https://github.com/facebook/rocksdb/blob/master/include/rocksdb/utilities/transaction_db.h#L398
    // Caller is responsible for deleting the returned transaction when no longer needed.
    auto txn = std::unique_ptr<rocksdb::Transaction>(get_txn_by_name(txn_name));
    auto s = txn->Commit();
    handle_rdb_error(s);
}

void rdb_wrapper_t::close()
{
    if (m_txn_db)
    {
        // Although we could have best effort close(), lets
        // handle any returned failure.
        auto s = m_txn_db->Close();
        handle_rdb_error(s);
    }
}

rocksdb::Iterator* rdb_wrapper_t::get_iterator()
{
    ASSERT_PRECONDITION(m_txn_db != nullptr, c_message_rocksdb_not_initialized);
    return m_txn_db->NewIterator(rocksdb::ReadOptions());
}

void rdb_wrapper_t::destroy_persistent_store()
{
    rocksdb::DestroyDB(m_data_dir, rocksdb::Options{});
}

bool rdb_wrapper_t::is_db_open()
{
    return bool(m_txn_db);
}

rocksdb::Transaction* rdb_wrapper_t::get_txn_by_name(const std::string& txn_name)
{
    ASSERT_PRECONDITION(m_txn_db != nullptr, c_message_rocksdb_not_initialized);
    return m_txn_db->GetTransactionByName(txn_name);
}

void rdb_wrapper_t::handle_rdb_error(rocksdb::Status status)
{
    // Todo (Mihir) Additionally log status information.
    if (!status.ok())
    {
        throw persistent_store_error(status.getState(), status.code());
    }
}

} // namespace db
} // namespace gaia
