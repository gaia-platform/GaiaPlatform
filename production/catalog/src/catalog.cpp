/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/catalog.hpp"

#include <optional>

#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/catalog/ddl_executor.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/exceptions.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace catalog
{

forbidden_system_db_operation_internal::forbidden_system_db_operation_internal(const std::string& name)
{
    m_message = "'" + name + "' is a system database. Operations on system databases are not allowed.";
}

db_already_exists_internal::db_already_exists_internal(const std::string& name)
{
    std::stringstream message;
    message << "A database with the name '" << name << "' already exists.";
    m_message = message.str();
}

db_does_not_exist_internal::db_does_not_exist_internal(const std::string& name)
{
    std::stringstream message;
    message << "The database '" << name << "' does not exist.";
    m_message = message.str();
}

table_already_exists_internal::table_already_exists_internal(const std::string& name)
{
    std::stringstream message;
    message << "A table with the name '" << name << "' already exists.";
    m_message = message.str();
}

table_does_not_exist_internal::table_does_not_exist_internal(const std::string& name)
{
    std::stringstream message;
    message << "The table '" << name << "' does not exist.";
    m_message = message.str();
}

duplicate_field_internal::duplicate_field_internal(const std::string& name)
{
    std::stringstream message;
    message << "The field '" << name << "' is specified more than once.";
    m_message = message.str();
}

field_does_not_exist_internal::field_does_not_exist_internal(const std::string& name)
{
    std::stringstream message;
    message << "The field '" << name << "' does not exist.";
    m_message = message.str();
}

max_reference_count_reached_internal::max_reference_count_reached_internal()
{
    m_message = "Cannot add any more relationships because the maximum number of references has been reached!";
}

referential_integrity_violation_internal::referential_integrity_violation_internal(const std::string& message)
{
    m_message = message;
}

referential_integrity_violation_internal referential_integrity_violation_internal::drop_referenced_table(
    const std::string& referenced_table,
    const std::string& referencing_table)
{
    std::stringstream message;
    message
        << "Cannot drop table '" << referenced_table
        << "' because it is referenced by table '" << referencing_table << "'.";
    return referential_integrity_violation_internal{message.str()};
}

relationship_already_exists_internal::relationship_already_exists_internal(const std::string& name)
{
    std::stringstream message;
    message << "A relationship with the name '" << name << "' already exists.";
    m_message = message.str();
}

relationship_does_not_exist_internal::relationship_does_not_exist_internal(const std::string& name)
{
    std::stringstream message;
    message << "The relationship '" << name << "' does not exist.";
    m_message = message.str();
}

no_cross_db_relationship_internal::no_cross_db_relationship_internal(const std::string& name)
{
    std::stringstream message;
    message << "'" + name
            + "' defines a relationship across databases. Relationships across databases are not allowed";
    m_message = message.str();
}

relationship_tables_do_not_match_internal::relationship_tables_do_not_match_internal(
    const std::string& relationship,
    const std::string& name1,
    const std::string& name2)
{
    std::stringstream message;
    message
        << "The table '" << name1 << "' does not match the table '" << name2 << "' "
        << "in the relationship '" << relationship << "' definition.";
    m_message = message.str();
}

many_to_many_not_supported_internal::many_to_many_not_supported_internal(const std::string& relationship)
{
    std::stringstream message;
    message << "'" + relationship
            + "' defines a many-to-many relationship. Many-to-many relationships are not supported.";
    m_message = message.str();
}

many_to_many_not_supported_internal::many_to_many_not_supported_internal(const std::string& table1, const std::string& table2)
{
    std::stringstream message;
    message
        << "The many-to-many relationship defined "
        << "between '" << table1 << "'  and '" << table2 << "' is not supported.";
    m_message = message.str();
}

index_already_exists_internal::index_already_exists_internal(const std::string& name)
{
    std::stringstream message;
    message << "The index '" << name << "' already exists.";
    m_message = message.str();
}

index_does_not_exist_internal::index_does_not_exist_internal(const std::string& name)
{
    std::stringstream message;
    message << "The index '" << name << "' does not exist.";
    m_message = message.str();
}

invalid_relationship_field_internal::invalid_relationship_field_internal(const std::string& message)
{
    m_message = message;
}

ambiguous_reference_definition_internal::ambiguous_reference_definition_internal(const std::string& table, const std::string& ref_name)
{
    std::stringstream message;
    message
        << "The reference '" << ref_name << "' definition "
        << "in table '" << table << "' has mutiple matching definitions.";
    m_message = message.str();
}

orphaned_reference_definition_internal::orphaned_reference_definition_internal(const std::string& table, const std::string& ref_name)
{
    std::stringstream message;
    message
        << "The reference '" << ref_name << "' definition "
        << "in table '" << table << "' has no matching definition.";
    m_message = message.str();
}

invalid_create_list_internal::invalid_create_list_internal(const std::string& message)
{
    m_message = "Invalid create statment in a list: ";
    m_message += message;
}

inline void check_not_system_db(const string& name)
{
    if (name == c_catalog_db_name || name == c_event_log_db_name)
    {
        throw forbidden_system_db_operation_internal(name);
    }
}

// The initialize the core catalog tables, then the rule catalog tables.
void initialize_catalog()
{
    bool throw_on_exists = false;
    bool auto_drop = false;

    std::optional<constraint_list_t> constraint = constraint_list_t{};
    constraint->emplace_back(make_unique<ddl::constraint_t>(ddl::constraint_type_t::unique));

    // The core catalog tables are initialized once during the first call to create_table().
    // After the core tables are present, these rule tables can be defined.

    // Conventions:
    //   - Field names ending in "_name" are used to store the VLR reference to the parent row
    //     of the named type, e.g. gaia_ruleset_name will match a row of the gaia_ruleset table.
    //   - Field names ending in "_id" are used to store the gaia_id_t of a row of the
    //     named type. These are used only to point from rule tables to core tables.
    //   - Fields names not ending in either "_id" or "_name" are data in the row, not used
    //     to create connections with other rows.
    //   - The connection-related fields are placed below the data fields.

    // gaia_ruleset_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1, constraint));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("serial_stream", data_type_t::e_string, 1));
        create_table(c_catalog_db_name, c_gaia_ruleset_table_name, fields, throw_on_exists, auto_drop);
    }

    // gaia_rule_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1, constraint));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_ruleset_name", data_type_t::e_string, 1));
        create_table(c_catalog_db_name, c_gaia_rule_table_name, fields, throw_on_exists, auto_drop);
    }

    // gaia_application_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1, constraint));
        create_table(c_catalog_db_name, c_gaia_application_table_name, fields, throw_on_exists, auto_drop);
    }

    // app_database_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_application_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_database_id", data_type_t::e_uint64, 1));
        create_table(c_catalog_db_name, c_app_database_table_name, fields, throw_on_exists, auto_drop);
    }

    // app_ruleset_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("active_on_startup", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_application_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_ruleset_name", data_type_t::e_string, 1));
        create_table(c_catalog_db_name, c_app_ruleset_table_name, fields, throw_on_exists, auto_drop);
    }

    // ruleset_database_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_ruleset_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_database_id", data_type_t::e_uint64, 1));
        create_table(c_catalog_db_name, c_ruleset_database_table_name, fields, throw_on_exists, auto_drop);
    }

    // rule_table_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("anchor", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_rule_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_table_id", data_type_t::e_uint64, 1));
        create_table(c_catalog_db_name, c_rule_table_table_name, fields, throw_on_exists, auto_drop);
    }

    // rule_field_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("active", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_rule_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_field_id", data_type_t::e_uint64, 1));
        create_table(c_catalog_db_name, c_rule_field_table_name, fields, throw_on_exists, auto_drop);
    }

    // rule_relationship_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_rule_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("gaia_relationship_id", data_type_t::e_uint64, 1));
        create_table(c_catalog_db_name, c_rule_relationship_table_name, fields, throw_on_exists, auto_drop);
    }

    // gaia_ruleset_t -> gaia_rule_t
    {
        vector<string> parent_fields{"name"};
        vector<string> child_fields{"gaia_ruleset_name"};
        ddl::table_field_map_t value_link{{c_catalog_db_name, c_gaia_ruleset_table_name, parent_fields}, {c_catalog_db_name, c_gaia_rule_table_name, child_fields}};
        create_relationship(
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
        create_relationship(
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
        create_relationship(
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
        create_relationship(
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
        create_relationship(
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
        create_relationship(
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
        create_relationship(
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
        create_relationship(
            "rule_catalog_gaia_rule_gaia_rule_rule_relationship",
            {c_catalog_db_name, c_gaia_rule_table_name, "rule_relationships", c_catalog_db_name, c_rule_relationship_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_rule_relationship_table_name, "rule", c_catalog_db_name, c_gaia_rule_table_name, relationship_cardinality_t::one},
            value_link,
            false,
            false);
    }
}

void use_database(const string& name)
{
    check_not_system_db(name);
    direct_access::auto_transaction_t txn(false);
    ddl_executor_t::get().switch_db_context(name);
}

gaia_id_t create_database(const string& name, bool throw_on_exists, bool auto_drop)
{
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    gaia_id_t id = ddl_executor.create_database(name, throw_on_exists, auto_drop);
    txn.commit();
    return id;
}

gaia_id_t create_table(const string& name, const ddl::field_def_list_t& fields)
{
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    gaia_id_t id = ddl_executor.create_table("", name, fields);
    txn.commit();
    return id;
}

gaia_id_t create_table(
    const string& db_name,
    const string& name,
    const ddl::field_def_list_t& fields,
    bool throw_on_exists,
    bool auto_drop)
{
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    check_not_system_db(name);
    gaia_id_t id = ddl_executor.create_table(db_name, name, fields, throw_on_exists, auto_drop);
    txn.commit();
    return id;
}

gaia_id_t create_relationship(
    const string& name,
    const ddl::link_def_t& link1,
    const ddl::link_def_t& link2,
    bool throw_on_exists,
    bool auto_drop)
{
    return create_relationship(name, link1, link2, std::nullopt, throw_on_exists, auto_drop);
}

gaia_id_t create_relationship(
    const string& name,
    const ddl::link_def_t& link1,
    const ddl::link_def_t& link2,
    const optional<ddl::table_field_map_t>& field_map,
    bool throw_on_exists,
    bool auto_drop)
{
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    gaia_id_t id = ddl_executor.create_relationship(name, link1, link2, field_map, throw_on_exists, auto_drop);
    txn.commit();
    return id;
}

gaia_id_t create_index(
    const std::string& index_name,
    bool unique,
    index_type_t type,
    const std::string& db_name,
    const std::string& table_name,
    const std::vector<std::string>& field_names,
    bool throw_on_exists,
    bool auto_drop)
{
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    gaia_id_t id = ddl_executor.create_index(
        index_name, unique, type, db_name, table_name, field_names, throw_on_exists, auto_drop);
    txn.commit();
    return id;
}

void drop_relationship(const string& name, bool throw_unless_exists)
{
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    ddl_executor.drop_relationship(name, throw_unless_exists);
    txn.commit();
}

void drop_index(const string& name, bool throw_unless_exists)
{
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    ddl_executor.drop_index(name, throw_unless_exists);
    txn.commit();
}

void drop_database(const string& name, bool throw_unless_exists)
{
    check_not_system_db(name);
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    ddl_executor.drop_database(name, throw_unless_exists);
    txn.commit();
}

void drop_table(const string& name, bool throw_unless_exists)
{
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    ddl_executor.drop_table("", name, throw_unless_exists);
    txn.commit();
}

void drop_table(const string& db_name, const string& name, bool throw_unless_exists)
{
    check_not_system_db(name);
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    ddl_executor.drop_table(db_name, name, throw_unless_exists);
    txn.commit();
}

gaia_id_t find_db_id(const string& db_name)
{
    ddl_executor_t& ddl_executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    return ddl_executor.find_db_id(db_name);
}

vector<gaia_id_t> list_fields(gaia_id_t table_id)
{
    vector<gaia_id_t> fields;
    // Direct access reference list API guarantees LIFO. As long as we only
    // allow appending new fields to table definitions, reversing the field list
    // order should result in fields being listed in the ascending order of
    // their positions.
    for (const auto& field : gaia_table_t::get(table_id).gaia_fields())
    {
        fields.insert(fields.begin(), field.gaia_id());
    }
    return fields;
}

vector<gaia_id_t> list_references(gaia_id_t table_id)
{
    return list_child_relationships(table_id);
}

vector<gaia_id_t> list_child_relationships(gaia_id_t table_id)
{
    vector<gaia_id_t> relationships;

    for (const gaia_relationship_t& child_relationship :
         gaia_table_t::get(table_id).incoming_relationships())
    {
        relationships.push_back(child_relationship.gaia_id());
    }

    return relationships;
}

vector<gaia_id_t> list_parent_relationships(gaia_id_t table_id)
{
    vector<gaia_id_t> relationships;

    for (const gaia_relationship_t& parent_relationship :
         gaia_table_t::get(table_id).outgoing_relationships())
    {
        relationships.push_back(parent_relationship.gaia_id());
    }

    return relationships;
}

} // namespace catalog
} // namespace gaia
