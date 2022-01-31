/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <optional>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/ddl_executor.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/exceptions.hpp"

#include "event_manager.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::catalog::ddl;

namespace gaia
{
namespace rules
{

// The initialize the core catalog tables, then the rule catalog tables.
void event_manager_t::initialize_rule_tables()
{
    bool throw_on_exists = false;
    bool auto_drop = false;

    std::optional<constraint_list_t> constraint = constraint_list_t{};
    constraint->emplace_back(make_unique<constraint_t>(constraint_type_t::unique));

    ddl_executor_t& ddl_executor = ddl_executor_t::get();

    // Conventions:
    //   - Field names ending in "_name" are used to store the VLR reference to the parent row
    //     of the named type, e.g. gaia_ruleset_name will match a row of the gaia_ruleset table.
    //   - Field names ending in "_id" are used to store the gaia_id_t of a row of the
    //     named type. These are used only to point from rule tables to core tables.
    //   - Fields names not ending in either "_id" or "_name" are data in the row, not used
    //     to create connections with other rows.
    //   - The connection-related fields are placed below the data fields.

    db::begin_transaction();

    // gaia_ruleset_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1, constraint));
        fields.emplace_back(make_unique<data_field_def_t>("serial_stream", data_type_t::e_string, 1));
        ddl_executor.create_table(c_catalog_db_name, c_gaia_ruleset_table_name, fields, throw_on_exists, auto_drop);
    }

    // gaia_rule_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1, constraint));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_ruleset_name", data_type_t::e_string, 1));
        ddl_executor.create_table(c_catalog_db_name, c_gaia_rule_table_name, fields, throw_on_exists, auto_drop);
    }

    // gaia_application_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1, constraint));
        ddl_executor.create_table(c_catalog_db_name, c_gaia_application_table_name, fields, throw_on_exists, auto_drop);
    }

    // app_database_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("gaia_application_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_database_id", data_type_t::e_uint64, 1));
        ddl_executor.create_table(c_catalog_db_name, c_app_database_table_name, fields, throw_on_exists, auto_drop);
    }

    // app_ruleset_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("active_on_startup", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_application_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_ruleset_name", data_type_t::e_string, 1));
        ddl_executor.create_table(c_catalog_db_name, c_app_ruleset_table_name, fields, throw_on_exists, auto_drop);
    }

    // ruleset_database_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("gaia_ruleset_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_database_id", data_type_t::e_uint64, 1));
        ddl_executor.create_table(c_catalog_db_name, c_ruleset_database_table_name, fields, throw_on_exists, auto_drop);
    }

    // rule_table_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<data_field_def_t>("anchor", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_rule_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_table_id", data_type_t::e_uint64, 1));
        ddl_executor.create_table(c_catalog_db_name, c_rule_table_table_name, fields, throw_on_exists, auto_drop);
    }

    // rule_field_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<data_field_def_t>("active", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_rule_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_field_id", data_type_t::e_uint64, 1));
        ddl_executor.create_table(c_catalog_db_name, c_rule_field_table_name, fields, throw_on_exists, auto_drop);
    }

    // rule_relationship_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_rule_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("gaia_relationship_id", data_type_t::e_uint64, 1));
        ddl_executor.create_table(c_catalog_db_name, c_rule_relationship_table_name, fields, throw_on_exists, auto_drop);
    }

    // gaia_ruleset_t -> gaia_rule_t
    {
        vector<string> parent_fields{"name"};
        vector<string> child_fields{"gaia_ruleset_name"};
        ddl::table_field_map_t value_link{{c_catalog_db_name, c_gaia_ruleset_table_name, parent_fields}, {c_catalog_db_name, c_gaia_rule_table_name, child_fields}};
        ddl_executor.create_relationship(
            "rule_catalog_gaia_ruleset_gaia_rule",
            {c_catalog_db_name, c_gaia_ruleset_table_name, "gaia_rules", c_catalog_db_name, c_gaia_rule_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_gaia_rule_table_name, "ruleset", c_catalog_db_name, c_gaia_ruleset_table_name, relationship_cardinality_t::one},
            value_link,
            false,
            false);
    }

    // gaia_application_t -> app_database_t
    {
        vector<string> parent_fields{"name"};
        vector<string> child_fields{"gaia_application_name"};
        ddl::table_field_map_t value_link{{c_catalog_db_name, c_gaia_application_table_name, parent_fields}, {c_catalog_db_name, c_app_database_table_name, child_fields}};
        ddl_executor.create_relationship(
            "rule_catalog_gaia_application_app_database",
            {c_catalog_db_name, c_gaia_application_table_name, "app_databases", c_catalog_db_name, c_app_database_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_app_database_table_name, "application", c_catalog_db_name, c_gaia_application_table_name, relationship_cardinality_t::one},
            value_link,
            false,
            false);
    }

    // gaia_application_t -> app_ruleset_t
    {
        vector<string> parent_fields{"name"};
        vector<string> child_fields{"gaia_application_name"};
        ddl::table_field_map_t value_link{{c_catalog_db_name, c_gaia_application_table_name, parent_fields}, {c_catalog_db_name, c_app_ruleset_table_name, child_fields}};
        ddl_executor.create_relationship(
            "rule_catalog_gaia_application_app_ruleset",
            {c_catalog_db_name, c_gaia_application_table_name, "app_rulesets", c_catalog_db_name, c_app_ruleset_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_app_ruleset_table_name, "application", c_catalog_db_name, c_gaia_application_table_name, relationship_cardinality_t::one},
            value_link,
            false,
            false);
    }

    // gaia_ruleset_t -> ruleset_database_t
    {
        vector<string> parent_fields{"name"};
        vector<string> child_fields{"gaia_ruleset_name"};
        ddl::table_field_map_t value_link{{c_catalog_db_name, c_gaia_ruleset_table_name, parent_fields}, {c_catalog_db_name, c_ruleset_database_table_name, child_fields}};
        ddl_executor.create_relationship(
            "rule_catalog_gaia_ruleset_ruleset_database",
            {c_catalog_db_name, c_gaia_ruleset_table_name, "ruleset_databases", c_catalog_db_name, c_ruleset_database_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_ruleset_database_table_name, "ruleset", c_catalog_db_name, c_gaia_ruleset_table_name, relationship_cardinality_t::one},
            value_link,
            false,
            false);
    }

    // gaia_ruleset_t -> app_ruleset_t
    {
        vector<string> parent_fields{"name"};
        vector<string> child_fields{"gaia_ruleset_name"};
        ddl::table_field_map_t value_link{{c_catalog_db_name, c_gaia_ruleset_table_name, parent_fields}, {c_catalog_db_name, c_app_ruleset_table_name, child_fields}};
        ddl_executor.create_relationship(
            "rule_catalog_gaia_ruleset_app_ruleset",
            {c_catalog_db_name, c_gaia_ruleset_table_name, "app_rulesets", c_catalog_db_name, c_app_ruleset_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_app_ruleset_table_name, "ruleset", c_catalog_db_name, c_gaia_ruleset_table_name, relationship_cardinality_t::one},
            value_link,
            false,
            false);
    }

    // gaia_rule_t -> rule_table_t
    {
        vector<string> parent_fields{"name"};
        vector<string> child_fields{"gaia_rule_name"};
        ddl::table_field_map_t value_link{{c_catalog_db_name, c_gaia_rule_table_name, parent_fields}, {c_catalog_db_name, c_rule_table_table_name, child_fields}};
        ddl_executor.create_relationship(
            "rule_catalog_gaia_rule_gaia_rule_rule_table",
            {c_catalog_db_name, c_gaia_rule_table_name, "rule_tables", c_catalog_db_name, c_rule_table_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_rule_table_table_name, "rule", c_catalog_db_name, c_gaia_rule_table_name, relationship_cardinality_t::one},
            value_link,
            false,
            false);
    }

    // gaia_rule_t -> rule_field_t
    {
        vector<string> parent_fields{"name"};
        vector<string> child_fields{"gaia_rule_name"};
        ddl::table_field_map_t value_link{{c_catalog_db_name, c_gaia_rule_table_name, parent_fields}, {c_catalog_db_name, c_rule_field_table_name, child_fields}};
        ddl_executor.create_relationship(
            "rule_catalog_gaia_rule_gaia_rule_field_table",
            {c_catalog_db_name, c_gaia_rule_table_name, "rule_fields", c_catalog_db_name, c_rule_field_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_rule_field_table_name, "rule", c_catalog_db_name, c_gaia_rule_table_name, relationship_cardinality_t::one},
            value_link,
            false,
            false);
    }

    // gaia_rule_t -> rule_relationship_t
    {
        vector<string> parent_fields{"name"};
        vector<string> child_fields{"gaia_rule_name"};
        ddl::table_field_map_t value_link{{c_catalog_db_name, c_gaia_rule_table_name, parent_fields}, {c_catalog_db_name, c_rule_relationship_table_name, child_fields}};
        ddl_executor.create_relationship(
            "rule_catalog_gaia_rule_gaia_rule_rule_relationship",
            {c_catalog_db_name, c_gaia_rule_table_name, "rule_relationships", c_catalog_db_name, c_rule_relationship_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_rule_relationship_table_name, "rule", c_catalog_db_name, c_gaia_rule_table_name, relationship_cardinality_t::one},
            value_link,
            false,
            false);
    }

    db::commit_transaction();
}

} // namespace rules
} // namespace gaia
