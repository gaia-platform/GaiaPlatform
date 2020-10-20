/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "db_types.hpp"
#include "gaia_common.hpp"

// This file provides gaia specific functionality to persist writes to & read from
// RocksDB during recovery.
// This file will be called by the storage engine & leverages the simple RocksDB internal library (rdb_internal.hpp)
namespace gaia
{
namespace db
{

constexpr size_t c_max_open_db_attempt_count = 10;

class rdb_internal_t;

class persistent_store_manager
{
private:
    static std::unique_ptr<gaia::db::rdb_internal_t> rdb_internal;

public:
    persistent_store_manager();
    ~persistent_store_manager();

    /**
     * Open rocksdb with the correct options.
     */
    void open();

    /**
     * Close the database.
     */
    void close();

    /**
     * Iterate over all elements in the LSM and call SE create API
     * for every key/value pair obtained (after deduping keys).
     */
    void recover();

    std::string begin_txn(gaia::db::gaia_txn_id_t txn_id);

    /**
     * This method will serialize the transaction to the log.
     * We expect writes to the RocksDB WAL to just work; this
     * method will sigabrt otherwise.
     */
    void prepare_wal_for_write(std::string& txn_name);

    /**
     * This method will append a commit marker with the appropriate
     * txn_id to the log, and will additionally insert entries
     * into the RocksDB write buffer (which then writes KV's to disk on getting full)
     *
     * We expect writes to the RocksDB WAL to just work; this
     * method will sigabrt otherwise. This also covers the case where writing
     * to the log succeeds but writing to the RocksDB memory buffer fails for any reason -
     * leading to incomplete buffer writes.
     *
     * The RocksDB commit API will additionally perform its own validation, but this codepath
     * has been switched off so we don't expect any errors from the normal flow of execution.
     */
    void append_wal_commit_marker(std::string& txn_name);

    /**
     * Append a rollback marker to the log.
     */
    void append_wal_rollback_marker(std::string& txn_name);

    /**
     * Destroy the persistent store.
     */
    void destroy_persistent_store();

    /**
     * This method is used to create a Gaia object from a decoded RocksDB key-value pair.
     */
    static void create_object_on_recovery(
        gaia::common::gaia_id_t id,
        gaia::common::gaia_type_t type,
        size_t num_refs,
        size_t data_size,
        const void* data);
};

} // namespace db
} // namespace gaia
