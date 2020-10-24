/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_catalog.hpp"

#include "ddl_executor.hpp"
#include "gaia_catalog.h"

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
        if (field.type() != static_cast<uint8_t>(data_type_t::e_references))
        {
            fields.insert(fields.begin(), field.gaia_id());
        }
    }
    return fields;
}

vector<gaia_id_t> list_references(gaia_id_t table_id)
{
    vector<gaia_id_t> references;
    // Direct access reference list API guarantees LIFO. As long as we only
    // allow appending new references to table definitions, reversing the
    // reference field list order should result in references being listed in
    // the ascending order of their positions.
    for (const auto& field : gaia_table_t::get(table_id).gaia_field_list())
    {
        if (field.type() == static_cast<uint8_t>(data_type_t::e_references))
        {
            references.insert(references.begin(), field.gaia_id());
        }
    }
    return references;
}

} // namespace catalog
} // namespace gaia
