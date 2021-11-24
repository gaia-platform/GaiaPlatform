/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/catalog.hpp"

#include <optional>

#include "gaia_internal/catalog/ddl_executor.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace catalog
{

inline void check_not_system_db(const string& name)
{
    if (name == c_catalog_db_name || name == c_event_log_db_name)
    {
        throw forbidden_system_db_operation(name);
    }
}

void initialize_catalog()
{
    ddl_executor_t::get();
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
