/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"

#include "gaia_internal/db/db_types.hpp"

#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_shared_data.hpp"
#include "rdb_internal.hpp"

// This file provides gaia specific functionality to persist writes to & read from
// RocksDB during recovery.
// This file will be called by the database and leverages the simple RocksDB internal library (rdb_internal.hpp)
namespace gaia
{
namespace db
{

class persistent_store_manager
{

public:
    persistent_store_manager(
        gaia::db::counters_t* counters, gaia::db::locators_t* locators, std::string data_dir);
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

    /**
     * This API is only used during checkpointing & recovery.
     */
    void put(gaia::db::db_object_t& object);

    /**
     * This API is only used during checkpointing & recovery.
     */
    void remove(gaia::common::gaia_id_t id_to_remove);

    /**
     * Flush rocksdb memory buffer to disk as an SST file.
     */
    void flush();

    /**
     * Destroy the persistent store.
     */
    void destroy_persistent_store();

    /**
     * Get custom key. Used to retain a gaia counter across restarts.
     */
    uint64_t get_value(const std::string& key);

    /**
     * Update custom key's value. Used to retain gaia counter across restarts.
     */
    void update_value(const std::string& key, uint64_t value_to_write);

    static constexpr char c_data_dir_command_flag[] = "--data-dir";
    static constexpr char c_persistent_store_dir_name[] = "/data";
    static const std::string c_last_processed_log_num_key;

private:
    gaia::db::counters_t* m_counters = nullptr;
    gaia::db::locators_t* m_locators = nullptr;
    std::unique_ptr<gaia::db::rdb_internal_t> m_rdb_internal;
    std::string m_data_dir_path;
};

} // namespace db
} // namespace gaia
