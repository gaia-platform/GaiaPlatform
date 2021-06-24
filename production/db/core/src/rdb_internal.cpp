/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rdb_internal.hpp"

#include <iostream>
#include <sstream>
#include <string>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction_db.h"
#include "rocksdb/write_batch.h"

#include "gaia_internal/common/persistent_store_error.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{

static const std::string c_message_rocksdb_not_initialized = "RocksDB database is not initialized.";

void rdb_internal_t::open_db(const rocksdb::Options& init_options)
{
    // RocksDB throws an IOError (Lock on persistent dir) when trying to open (recover) twice on the same directory
    // while a process is already up.
    // The same error is also seen when reopening the db after a large volume of deletes
    // See https://github.com/facebook/rocksdb/issues/4421
    rocksdb::DB* db;
    rocksdb::Status s;

    s = rocksdb::DB::Open(init_options, m_data_dir, &db);
    if (s.ok())
    {
        m_db.reset(db);
    }
    else
    {
        handle_rdb_error(s);
    }
    ASSERT_POSTCONDITION(m_db != nullptr, c_message_rocksdb_not_initialized);
}

void rdb_internal_t::close()
{
    if (m_db)
    {
        // Although we could have best effort close(), lets
        // handle any returned failure.
        auto s = m_db->Close();
        handle_rdb_error(s);
    }
}

rocksdb::Iterator* rdb_internal_t::get_iterator()
{
    ASSERT_PRECONDITION(m_db != nullptr, c_message_rocksdb_not_initialized);
    return m_db->NewIterator(rocksdb::ReadOptions());
}

void rdb_internal_t::destroy_persistent_store()
{
    rocksdb::DestroyDB(m_data_dir, rocksdb::Options{});
}

void rdb_internal_t::flush()
{
    rocksdb::FlushOptions options{};
    m_db->Flush(options);
}

void rdb_internal_t::put(const rocksdb::Slice& key, const rocksdb::Slice& value)
{
    m_db->Put(m_write_options, key, value);
}

void rdb_internal_t::remove(const rocksdb::Slice& key)
{
    m_db->Delete(m_write_options, key);
}

void rdb_internal_t::get(const rocksdb::Slice& key, rocksdb::Slice& value)
{
    std::string val;
    rocksdb::Status status = m_db->Get(rocksdb::ReadOptions(), key, &val);
    std::cout << "READ VAL = " << val << std::endl;
    if (status.IsNotFound())
    {
        // Not found.
        value = rocksdb::Slice();
    }
    else if (status.ok())
    {
        value = rocksdb::Slice(val);
        std::cout << "READ VALUE = " << value.data() << std::endl;
    }
    else
    {
        handle_rdb_error(status);
    }
}

bool rdb_internal_t::is_db_open()
{
    return bool(m_db);
}

void rdb_internal_t::handle_rdb_error(rocksdb::Status status)
{
    // Todo (Mihir) Additionally log status information.
    if (!status.ok())
    {
        throw persistent_store_error(status.getState(), status.code());
    }
}

} // namespace db
} // namespace gaia
