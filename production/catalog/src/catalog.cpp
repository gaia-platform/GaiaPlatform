/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/catalog.hpp"

#include "gaia_internal/catalog/ddl_executor.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace catalog
{

void initialize_catalog()
{
    ddl_executor_t::get();
}

void use_database(const string& name)
{
    ddl_executor_t::get().switch_db_context(name);
}

gaia_id_t create_database(const string& name, bool throw_on_exists)
{
    return ddl_executor_t::get().create_database(name, throw_on_exists);
}

gaia_id_t create_table(const string& name, const ddl::field_def_list_t& fields)
{
    return ddl_executor_t::get().create_table("", name, fields);
}

gaia_id_t create_table(
    const string& db_name,
    const string& name,
    const ddl::field_def_list_t& fields,
    bool throw_on_exists)
{
    return ddl_executor_t::get().create_table(db_name, name, fields, throw_on_exists);
}

gaia_id_t create_relationship(
    const string& name,
    const ddl::link_def_t& link1,
    const ddl::link_def_t& link2,
    bool throw_on_exists)
{
    return ddl_executor_t::get().create_relationship(name, link1, link2, throw_on_exists);
}

void drop_database(const string& name, bool throw_unless_exists)
{
    return ddl_executor_t::get().drop_database(name, throw_unless_exists);
}

void drop_table(const string& name, bool throw_unless_exists)
{
    return ddl_executor_t::get().drop_table("", name, throw_unless_exists);
}

void drop_table(const string& db_name, const string& name, bool throw_unless_exists)
{
    return ddl_executor_t::get().drop_table(db_name, name, throw_unless_exists);
}

gaia_id_t find_db_id(const string& db_name)
{
    return ddl_executor_t::get().find_db_id(db_name);
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
         gaia_table_t::get(table_id).gaia_relationships_child())
    {
        relationships.push_back(child_relationship.gaia_id());
    }

    return relationships;
}

vector<gaia_id_t> list_parent_relationships(gaia_id_t table_id)
{
    vector<gaia_id_t> relationships;

    for (const gaia_relationship_t& parent_relationship :
         gaia_table_t::get(table_id).gaia_relationships_parent())
    {
        relationships.push_back(parent_relationship.gaia_id());
    }

    return relationships;
}

} // namespace catalog
} // namespace gaia
