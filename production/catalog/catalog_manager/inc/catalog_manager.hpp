/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <shared_mutex>
#include <unordered_map>
#include <utility>

#include "gaia_catalog.hpp"
#include "gaia_boot.hpp"

namespace gaia {
namespace catalog {

using db_names_t = unordered_map<string, gaia_id_t>;
using table_names_t = unordered_map<string, gaia_id_t>;
using type_map_t = unordered_map<gaia_type_t, gaia_id_t>;

class catalog_manager_t {
  public:
    /**
     * Catalog manager scaffolding to ensure we have one global static instance
     */
    catalog_manager_t(catalog_manager_t &) = delete;
    void operator=(catalog_manager_t const &) = delete;
    static catalog_manager_t &get();

    /**
     * APIs for accessing catalog records
     */
    gaia_id_t create_database(const string &name, bool throw_on_exist = true);
    gaia_id_t create_table(const string &dbname, const string &name, const ddl::field_def_list_t &fields, bool throw_on_exist = true);
    void drop_table(const string &dbname, const string &name);

    gaia_id_t find_db_id(const string& dbname) const;
    gaia_id_t find_table_id(gaia_type_t);

    vector<gaia_id_t> list_fields(gaia_id_t table_id) const;
    vector<gaia_id_t> list_references(gaia_id_t table_id) const;

  private:
    // Only internal static creation is allowed
    catalog_manager_t();
    ~catalog_manager_t() {}

    // Initialize the catalog manager.
    void init();

    // This is the internal create table implementation.
    // The public create_table calls this method but does not allow specifying an ID.
    // The call to create a table with a given ID is only intended for system tables.
    // 'throw_on_exist' indicates if an exception should be thrown when the table of
    // the given name already exists.
    gaia_id_t create_table_impl(
        const string &database_name,
        const string &table_name,
        const ddl::field_def_list_t &fields,
        bool is_system = false,
        bool throw_on_exist = true,
        gaia_type_t type = INVALID_GAIA_TYPE);

    // Find the database ID given its name.
    // The method does not use a lock.
    inline gaia_id_t find_db_id_no_lock(const string& dbname) const;

    // Clear all the caches (only for testing purposes).
    void clear_cache();
    // Reload all the caches from catalog records in storage engine.
    void reload_cache();

    // Bootstrap catalog with its own tables.
    void bootstrap_catalog();

    // Create other system tables that need constant IDs.
    void create_system_tables();

    // Create a map that allows table definitions to found via their types.
    void create_type_map();

    // Maintain some in-memory cache for fast lookup.
    // This is only intended for single process usage.
    // We cannot guarantee the cache is consistent across mutiple processes.
    // We should switch to use value index when the feature is ready.
    db_names_t m_db_names;
    table_names_t m_table_names;
    type_map_t m_type_map;

    gaia_id_t m_global_db_id;

    // Use the lock to ensure exclusive access to caches.
    mutable shared_mutex m_lock;
};
} // namespace catalog
} // namespace gaia
