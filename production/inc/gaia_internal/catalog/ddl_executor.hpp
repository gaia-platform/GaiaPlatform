/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "gaia/common.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

namespace gaia
{
namespace catalog
{

using db_names_t = std::unordered_map<std::string, gaia::common::gaia_id_t>;
using table_names_t = std::unordered_map<std::string, gaia::common::gaia_id_t>;
using relationship_names_t = std::unordered_map<std::string, gaia::common::gaia_id_t>;

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
    gaia::common::gaia_id_t create_database(const std::string& name, bool throw_on_exist = true);

    gaia::common::gaia_id_t create_table(
        const std::string& db_name,
        const std::string& name,
        const ddl::field_def_list_t& fields,
        bool throw_on_exist = true);

    gaia::common::gaia_id_t create_relationship(
        const std::string& name,
        const ddl::link_def_t& link1,
        const ddl::link_def_t& link2,
        const std::optional<ddl::table_field_map_t>& field_map,
        bool throw_on_exists);

    gaia::common::gaia_id_t create_index(
        const std::string& index_name,
        bool unique,
        index_type_t type,
        const std::string& db_name,
        const std::string& table_name,
        const std::vector<std::string>& field_names,
        bool throw_on_exists = true);

    void drop_table(const std::string& db_name, const std::string& name, bool throw_unless_exists);

    void drop_database(const std::string& name, bool throw_unless_exists);

    void drop_table(const std::string& db_name, const std::string& name);
    void drop_database(const std::string& name);

    void switch_db_context(const std::string& db_name);

    gaia::common::gaia_id_t find_db_id(const std::string& dbname) const;

    // Initialize the catalog manager.
    void reset();

private:
    // Only internal static creation is allowed
    ddl_executor_t();

    ~ddl_executor_t() = default;

    // This is the internal create table implementation.
    // The public create_table calls this method but does not allow specifying an ID.
    // The call to create a table with a given ID is only intended for system tables.
    // 'throw_on_exist' indicates if an exception should be thrown when the table of
    // the given name already exists.
    gaia::common::gaia_id_t create_table_impl(
        const std::string& database_name,
        const std::string& table_name,
        const ddl::field_def_list_t& fields,
        bool is_log = false,
        bool throw_on_exist = true,
        gaia::common::gaia_type_t type = gaia::common::c_invalid_gaia_type);

    // Internal drop table implementation. Callers need to acquire a transaction
    // before calling this method.
    // If enforce_referential_integrity is false it does not check referential integrity, fails otherwise.
    void drop_table_no_txn(gaia::common::gaia_id_t table_id, bool enforce_referential_integrity);

    // Drops the relationships associated with this table.
    // If enforce_referential_integrity is false it does not check referential integrity, fails otherwise.
    void drop_relationships_no_txn(gaia::common::gaia_id_t table_id, bool enforce_referential_integrity);

    // Find the database ID given its name.
    // The method does not use a lock.
    inline gaia::common::gaia_id_t find_db_id_no_lock(const std::string& dbname) const;

    // Clear all the caches (only for testing purposes).
    void clear_cache();
    // Reload all the caches from catalog records in database.
    void reload_cache();

    // Bootstrap catalog with its own tables.
    void bootstrap_catalog();

    // Create other system tables that need constant IDs.
    void create_system_tables();

    // Get the full name for a table composed of db and table names.
    static inline std::string get_full_table_name(const std::string& db, const std::string& table);

    // Get the table id given the db and table names.
    inline common::gaia_id_t get_table_id(const std::string& db, const std::string& table);

    // Verifies that a newly generated reference offset is valid.
    // Throws an exception if the new offset was found to be invalid,
    // which would happen if we ran out of reference offsets.
    static void validate_new_reference_offset(common::reference_offset_t reference_offset);

    // The following are helper functions for calculating relationship offsets.
    // We use them to compute offset field values of the "gaia_relationship" table .
    //
    // Find the next available offset in a container's parent relationships.
    static common::reference_offset_t find_parent_available_offset(const gaia_table_t::outgoing_relationships_list_t& relationships);
    // Find the next available offset in a container's child relationships.
    static common::reference_offset_t find_child_available_offset(const gaia_table_t::incoming_relationships_list_t& relationships);
    // Find the next available offset in the relationships of the given table.
    static common::reference_offset_t find_available_offset(gaia::common::gaia_id_t table);

    // Find field IDs given a table ID and field names.
    static std::vector<gaia::common::gaia_id_t> find_table_field_ids(
        gaia::common::gaia_id_t table_id, const std::vector<std::string>& field_names);

    // Maintain some in-memory cache for fast lookup.
    // This is only intended for single process usage.
    // We cannot guarantee the cache is consistent across multiple processes.
    // We should switch to use value index when the feature is ready.
    db_names_t m_db_names;
    table_names_t m_table_names;
    relationship_names_t m_relationship_names;

    gaia::common::gaia_id_t m_empty_db_id;

    // The DB context defines the database in which an entity like a table, an
    // index, or a relationship will be referred to without a database name.
    std::string m_db_context;
    inline std::string in_context(const std::string& db)
    {
        return db.empty() ? m_db_context : db;
    }

    // Use the lock to ensure exclusive access to caches.
    mutable std::shared_mutex m_lock;
};
} // namespace catalog
} // namespace gaia
