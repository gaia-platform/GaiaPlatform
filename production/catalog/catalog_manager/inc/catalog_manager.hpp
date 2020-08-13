/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <mutex>
#include <unordered_map>
#include <utility>

#include "gaia_catalog.hpp"

namespace gaia {
namespace catalog {

using table_names_t = unordered_map<string, gaia_id_t>;
using table_fields_t = unordered_map<gaia_id_t, vector<gaia_id_t>>;

class catalog_manager_t {
  public:
    /**
     * Catalog manager scaffolding to ensure we have one global static instance
     */
    catalog_manager_t(catalog_manager_t &) = delete;
    void operator=(catalog_manager_t const &) = delete;
    static catalog_manager_t &get();

    // Initialize the catalog manager.
    void init();

    /**
     * APIs for accessing catalog records
     */
    gaia_id_t create_table(const string &name, const ddl::field_def_list_t &fields);
    void drop_table(const string &name);

    // The following methods provide convenient access to catalog caches.
    // Thread safety is only guranteed by the underlying containers.
    // They are NOT thread safe if you are creating/dropping/modifying tables concurrently.
    // Use direct access API with transactions to access catalog records thread-safely.
    const set<gaia_id_t> &list_tables() const;
    const vector<gaia_id_t> &list_fields(gaia_id_t table_id) const;
    const vector<gaia_id_t> &list_references(gaia_id_t table_id) const;

  private:
    // Only internal static creation is allowed
    catalog_manager_t() {}
    ~catalog_manager_t() {}

    // This is the internal create table implementation.
    // The public create_table calls this method but does not allow specifying an ID.
    // The call to create a table with a given ID is only intended for system tables.
    // 'throw_on_exist' indicates if an exception should be thrown when the table of
    // the given name already exists.
    gaia_id_t create_table_impl(const string &name,
        const ddl::field_def_list_t &fields,
        bool throw_on_exist = true,
        gaia_id_t id = INVALID_GAIA_ID);

    // Clear all the caches (only for testing purposes).
    void clear_cache();
    // Reload all the caches from catalog records in storage engine.
    void reload_cache();

    // Load the bootstrapped catalog content
    void bootstrap_catalog();

    // Maintain some in-memory cache for fast lookup.
    // This is only intended for single process usage.
    // We cannot guarantee the cache is consistent across mutiple processes.
    // We should switch to use value index when the feature is ready.
    table_names_t m_table_names;
    table_fields_t m_table_fields;
    table_fields_t m_table_references;
    set<gaia_id_t> m_table_ids;

    // Use the lock to ensure exclusive access to caches.
    mutex m_lock;
};
} // namespace catalog
} // namespace gaia
