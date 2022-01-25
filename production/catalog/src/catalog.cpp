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

void initialize_catalog()
{
    ddl_executor_t::get();

    // Add the rules catalog to the catalog.
    // ddl::parser_t parser;
    // parser.parse_string(
    //     "use catalog;"
    //     "create table gaia_application (name string);"
    //     "create table gaia_rule (name string);"
    //     "create table app_ruleset (active_on_startup bool);"
    //     "relationship gaia_catalog_ruleset_rule ( gaia_application.app_rulesets -> app_ruleset[], app_ruleset.gaia_application -> gaia_application );");
    // execute(parser.statements);

    // // This must be reset in order for the new DDL definitions to be in the right place.
    // use_database(c_default_db_name);
    bool throw_on_exists = false;
    bool auto_drop = false;

    // gaia_application_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1));
        create_table(c_catalog_db_name, "gaia_application", fields, throw_on_exists, auto_drop);
    }

    // gaia_ruleset_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<ddl::data_field_def_t>("serial_stream", data_type_t::e_string, 1));
        create_table(c_catalog_db_name, "gaia_ruleset", fields, throw_on_exists, auto_drop);
    }

    // gaia_rule_t
    {
        field_def_list_t fields;
        fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1));
        create_table(c_catalog_db_name, "gaia_rule", fields, throw_on_exists, auto_drop);
    }

    create_relationship(
        "rule_catalog_gaia_ruleset_gaia_rule",
        {c_catalog_db_name, "gaia_ruleset", "gaia_rules", "catalog", "gaia_rule", relationship_cardinality_t::many},
        {c_catalog_db_name, "gaia_rule", "ruleset", "catalog", "gaia_ruleset", relationship_cardinality_t::one},
        false);
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
