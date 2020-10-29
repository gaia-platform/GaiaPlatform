/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "gaia_boot.hpp"
#include "gaia_catalog.hpp"

namespace gaia
{
namespace catalog
{

using db_names_t = unordered_map<string, gaia::common::gaia_id_t>;
using table_names_t = unordered_map<string, gaia::common::gaia_id_t>;

class ddl_executor_t
{
public:
    /**
     * Catalog manager scaffolding to ensure we have one global static instance
     */
    ddl_executor_t(ddl_executor_t&) = delete;
    void operator=(ddl_executor_t const&) = delete;
    static ddl_executor_t& get();

    /**
     * APIs for accessing catalog records
     */
    gaia::common::gaia_id_t create_database(const string& name, bool throw_on_exist = true);
    gaia::common::gaia_id_t create_table(
        const string& db_name,
        const string& name,
        const ddl::field_def_list_t& fields,
        bool throw_on_exist = true);
    void drop_table(const string& db_name, const string& name);
    void drop_database(const string& name);

    gaia::common::gaia_id_t find_db_id(const string& dbname) const;

private:
    // Only internal static creation is allowed
    ddl_executor_t();
    ~ddl_executor_t() = default;

    // Initialize the catalog manager.
    void init();

    // This is the internal create table implementation.
    // The public create_table calls this method but does not allow specifying an ID.
    // The call to create a table with a given ID is only intended for system tables.
    // 'throw_on_exist' indicates if an exception should be thrown when the table of
    // the given name already exists.
    gaia::common::gaia_id_t create_table_impl(
        const string& database_name,
        const string& table_name,
        const ddl::field_def_list_t& fields,
        bool is_log = false,
        bool throw_on_exist = true,
        gaia_type_t type = INVALID_GAIA_TYPE);

    // Internal drop table implementation. Callers need to acquire a transaction
    // before calling this method.
    // If enforce_referential_integrity is false it does not check referential integrity, fails otherwise.
    void drop_table_no_txn(gaia::common::gaia_id_t table_id, bool enforce_referential_integrity);

    // Drops the relationships associated with this table.
    // If enforce_referential_integrity is false it does not check referential integrity, fails otherwise.
    void drop_relationships_no_txn(gaia::common::gaia_id_t table_id, bool enforce_referential_integrity);

    // Find the database ID given its name.
    // The method does not use a lock.
    inline gaia::common::gaia_id_t find_db_id_no_lock(const string& dbname) const;

    // Clear all the caches (only for testing purposes).
    void clear_cache();
    // Reload all the caches from catalog records in storage engine.
    void reload_cache();

    // Bootstrap catalog with its own tables.
    void bootstrap_catalog();

    // Create other system tables that need constant IDs.
    void create_system_tables();

    // Get the full name for a table composed of db and table names.
    static inline string get_full_table_name(const string& db, const string& table);

    // Find the next available offset in a container parent relationships
    template <typename T_parent_relationships>
    uint8_t find_parent_available_offset(T_parent_relationships& relationships);

    // Find the next available offset in a container child relationships
    template <typename T_child_relationships>
    uint8_t find_child_available_offset(T_child_relationships& relationships);

    // Find the next available offset in the relationships of the given table
    uint8_t find_available_offset(gaia::common::gaia_id_t table);

    // Maintain some in-memory cache for fast lookup.
    // This is only intended for single process usage.
    // We cannot guarantee the cache is consistent across mutiple processes.
    // We should switch to use value index when the feature is ready.
    db_names_t m_db_names;
    table_names_t m_table_names;

    gaia::common::gaia_id_t m_empty_db_id;

    // Use the lock to ensure exclusive access to caches.
    mutable shared_mutex m_lock;
};
} // namespace catalog
} // namespace gaia
