/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <string>

#include "catalog_internal.hpp"

namespace gaia
{
namespace catalog
{

/**
 * Generate a foreign table DDL from parsing the definition of a table.
 *
 * @param db_name database name
 * @param table_name table name
 * @param fields table fields parsing result bindings
 * @param server_name FDW server name
 * @return DDL string
 */
std::string generate_fdw_ddl(
    const std::string& table_name, const ddl::field_def_list_t& fields, const std::string& server_name);

} // namespace catalog
} // namespace gaia
