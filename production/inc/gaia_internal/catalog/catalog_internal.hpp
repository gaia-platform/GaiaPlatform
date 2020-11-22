/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <string>
#include <vector>

#include "gaia/db/catalog.hpp"

namespace gaia
{
namespace catalog
{

/**
 * Generate FlatBuffers schema (fbs) for all catalog tables in a given database.
 * No root type is specified in the generated schema.
 *
 * @param dbname database name
 * @return generated fbs string
 */
std::string generate_fbs(const std::string& dbname);

/**
 * Retrieve the binary FlatBuffers schema (bfbs) for a given table.
 *
 * @param table_id id of the table
 * @return bfbs
 */
std::vector<uint8_t> get_bfbs(gaia::common::gaia_id_t table_id);

/**
 * Retrieve the serialization template (bin) for a given table.
 *
 * @param table_id id of the table
 * @return bin
 */
std::vector<uint8_t> get_bin(gaia::common::gaia_id_t table_id);

/**
 * Generate a foreign table DDL from parsing the definition of a table.
 *
 * @param table_id table id
 * @param server_name FDW server name
 * @return DDL string
 */
std::string generate_fdw_ddl(
    common::gaia_id_t table_id, const std::string& server_name);

} // namespace catalog
} // namespace gaia
