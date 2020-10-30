/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_catalog.hpp"

namespace gaia
{
namespace catalog
{

/**
 * Generate a foreign table DDL from parsing the definition of a table.
 *
 * @param table_id table id
 * @param server_name FDW server name
 * @return DDL string
 */
string generate_fdw_ddl(
    common::gaia_id_t table_id, const string& server_name);

/**
 * Generate a foreign table DDL from parsing the definition of a table.
 *
 * @param db_name database name
 * @param table_name table name
 * @param fields table fields parsing result bindings
 * @param server_name FDW server name
 * @return DDL string
 */
string generate_fdw_ddl(
    const string& table_name, const ddl::field_def_list_t& fields, const string& server_name);

} // namespace catalog
} // namespace gaia
