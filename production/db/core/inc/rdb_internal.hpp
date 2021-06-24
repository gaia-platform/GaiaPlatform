/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "rocksdb/db.h"

#include "gaia_internal/db/db_types.hpp"

// Simple library over RocksDB APIs.
namespace gaia
{
namespace db
{

class rdb_internal_t
{
public:
    rdb_internal_t(const std::string& dir, const rocksdb::WriteOptions& write_opts)
        : m_db(nullptr), m_data_dir(dir), m_write_options(write_opts)
    {
    }

    ~rdb_internal_t()
    {
        close();
    }

    void open_db(const rocksdb::Options& init_options);

    void close();

    rocksdb::Iterator* get_iterator();

    void destroy_persistent_store();

    bool is_db_open();

    void handle_rdb_error(rocksdb::Status status);

    void flush();

    void put(const rocksdb::Slice& key, const rocksdb::Slice& value);

    void remove(const rocksdb::Slice& key);

    void get(const rocksdb::Slice& key, std::string& value);

private:
    std::unique_ptr<rocksdb::DB> m_db;
    std::string m_data_dir;
    rocksdb::WriteOptions m_write_options;
};

} // namespace db
} // namespace gaia
