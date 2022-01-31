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
    gaia::common::gaia_id_t create_database(
        const std::string& name,
        bool throw_on_exists = true,
        bool auto_drop = false);

    gaia::common::gaia_id_t create_table(
        const std::string& db_name,
        const std::string& name,
        const ddl::field_def_list_t& fields,
        bool throw_on_exists = true,
        bool auto_drop = false);

    gaia::common::gaia_id_t create_system_table(
        const std::string& db_name,
        const std::string& name,
        const ddl::field_def_list_t& fields,
        bool throw_on_exists = true,
        bool auto_drop = false);

    gaia::common::gaia_id_t create_relationship(
        const std::string& name,
        const ddl::link_def_t& link1,
        const ddl::link_def_t& link2,
        const std::optional<ddl::table_field_map_t>& field_map,
        bool throw_on_exists,
        bool auto_drop = false);

    gaia::common::gaia_id_t create_index(
        const std::string& index_name,
        bool unique,
        index_type_t type,
        const std::string& db_name,
        const std::string& table_name,
        const std::vector<std::string>& field_names,
        bool throw_on_exists = true,
        bool auto_drop = false);

    void drop_table(const std::string& db_name, const std::string& name, bool throw_unless_exists);

    void drop_database(const std::string& name, bool throw_unless_exists);

    void drop_relationship(const std::string& name, bool throw_unless_exists);

    void drop_index(const std::string& name, bool throw_unless_exists);

    void drop_table(const std::string& db_name, const std::string& name);

    void drop_database(const std::string& name);

    void switch_db_context(const std::string& db_name);

    gaia::common::gaia_id_t find_db_id(const std::string& dbname) const;

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
        bool is_system = false,
        bool throw_on_exists = true,
        bool auto_drop = false,
        gaia::common::gaia_type_t type = gaia::common::c_invalid_gaia_type);

    gaia::common::gaia_id_t create_index(
        const std::string& name,
        bool unique,
        index_type_t type,
        common::gaia_id_t table_id,
        const std::vector<common::gaia_id_t>& field_ids);

    // Internal drop table implementation. Callers need to acquire a transaction
    // before calling this method.
    // If enforce_referential_integrity is false it does not check referential integrity, fails otherwise.
    void drop_table(gaia::common::gaia_id_t table_id, bool enforce_referential_integrity);

    // Drop the relationships associated with this table. Do not check
    // referential integrity.
    void drop_relationships_no_ri(gaia::common::gaia_id_t table_id);

    // Bootstrap catalog with builtin databases and tables.
    void bootstrap_catalog();

    // Drop the given relationship without referential integrity check.
    static void drop_relationship_no_ri(gaia_relationship_t& relationship);

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

    gaia::common::gaia_id_t m_empty_db_id;

    // The DB context defines the database in which an entity like a table, an
    // index, or a relationship will be referred to without a database name.
    std::string m_db_context;
    inline std::string in_context(const std::string& db)
    {
        return db.empty() ? m_db_context : db;
    }
};
} // namespace catalog
} // namespace gaia
