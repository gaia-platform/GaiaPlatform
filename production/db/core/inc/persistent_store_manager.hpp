/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <memory>

#include "gaia/common.hpp"

#include "gaia_internal/db/db_types.hpp"

#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_shared_data.hpp"
#include "log_io.hpp"
#include "rdb_wrapper.hpp"

// This file provides gaia specific functionality to persist writes to & read from
// RocksDB during recovery.
// This file will be called by the database and leverages the simple RocksDB wrapper library (rdb_wrapper.hpp)
namespace gaia
{
namespace db
{
namespace persistence
{

class persistent_store_manager_t
{

public:
    persistent_store_manager_t(
        gaia::db::counters_t* counters, std::filesystem::path data_dir);
    ~persistent_store_manager_t();

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
    void prepare_wal_for_write(gaia::db::txn_log_t* log, const std::string& txn_name);

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
    void append_wal_commit_marker(const std::string& txn_name);

    /**
     * Append a rollback marker to the log.
     */
    void append_wal_rollback_marker(const std::string& txn_name);

    /**
     * Destroy the persistent store.
     */
    void destroy_persistent_store();

    void init_log(const std::string& data_dir, const int validate_persistence_batch_eventfd);

    void reset_log();

    /**
     * This API is only used during checkpointing & recovery.
     */
    void put(gaia::db::db_recovered_object_t& object);

    /**
     * This API is only used during checkpointing & recovery.
     */
    void remove(gaia::common::gaia_id_t id_to_remove);

    /**
     * Flush rocksdb memory buffer to disk as an SST file. 
     * The rocksdb memtable is used to hold writes before writing them to an SST file.
     * The alternative is the RocksDB SSTFileWriter. Both options require reserving memory to stage writes in
     * the SST format before writing to disk.
     * Additionally, in RocksDB, each column family has an individual set of 
     * SST files and the memtable. Thus, writing to multiple column families will require maintaining multiple SSTFileWriter objects. 
     * All of this is handled by the memtable already.
     * Note that the memtable is only written to during log checkpointing.  
     */
    void flush();

    /**
     * Get value of a custom key. Used to retain a gaia counter across restarts.
     */
    uint64_t get_counter_value(const std::string& key);

    /**
     * Update a custom key's value. Used to retain gaia counter across restarts.
     */
    void set_counter_value(const std::string& key, uint64_t value_to_write);

    static constexpr char c_data_dir_command_flag[] = "--data-dir";
    static inline const std::filesystem::path c_persistent_store_dir_name{"data"};

    // Keep track of the last read log file to avoid reading it repeatedly during checkpointing.
    static inline const std::string c_last_processed_log_seq_key = "gaia_last_processed_log_seq_key";

    // Default value for any counters (example: log file sequence number) stored in the persistent store.
    // If a key for the counter is not found, the default value is returned.
    static inline const uint64_t c_default_counter_value = 0;

private:
    gaia::db::counters_t* m_counters = nullptr;
    std::unique_ptr<gaia::db::persistence::rdb_wrapper_t> m_rdb_wrapper;
    std::filesystem::path m_data_dir_path;
};

} // namespace persistence
} // namespace db
} // namespace gaia
