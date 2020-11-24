/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/db/catalog.hpp"

#include "ddl_executor.hpp"
#include "gaia_catalog.h"

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

gaia_id_t create_database(const string& name, bool throw_on_exists)
{
    return ddl_executor_t::get().create_database(name, throw_on_exists);
}

gaia_id_t create_table(const string& name, const ddl::field_def_list_t& fields)
{
    return ddl_executor_t::get().create_table("", name, fields);
}

gaia_id_t create_table(
    const string& dbname,
    const string& name,
    const ddl::field_def_list_t& fields,
    bool throw_on_exists)
{
    return ddl_executor_t::get().create_table(dbname, name, fields, throw_on_exists);
}

void drop_database(const string& name)
{
    return ddl_executor_t::get().drop_database(name);
}

void drop_table(const string& name)
{
    return ddl_executor_t::get().drop_table("", name);
}

void drop_table(const string& dbname, const string& name)
{
    return ddl_executor_t::get().drop_table(dbname, name);
}

gaia_id_t find_db_id(const string& dbname)
{
    return ddl_executor_t::get().find_db_id(dbname);
}

vector<gaia_id_t> list_fields(gaia_id_t table_id)
{
    vector<gaia_id_t> fields;
    // Direct access reference list API guarantees LIFO. As long as we only
    // allow appending new fields to table definitions, reversing the field list
    // order should result in fields being listed in the ascending order of
    // their positions.
    for (const auto& field : gaia_table_t::get(table_id).gaia_field_list())
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
         gaia_table_t::get(table_id).child_gaia_relationship_list())
    {
        relationships.push_back(child_relationship.gaia_id());
    }

    return relationships;
}

vector<gaia_id_t> list_parent_relationships(gaia_id_t table_id)
{
    vector<gaia_id_t> relationships;

    for (const gaia_relationship_t& parent_relationship :
         gaia_table_t::get(table_id).parent_gaia_relationship_list())
    {
        relationships.push_back(parent_relationship.gaia_id());
    }

    return relationships;
}

} // namespace catalog
} // namespace gaia
