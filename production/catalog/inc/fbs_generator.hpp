/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <string>

#include "gaia_internal/catalog/catalog_internal.hpp"

namespace gaia
{
namespace catalog
{

/**
 * Generate FlatBuffers schema (fbs) for a catalog table.
 * The given table is the root type of the generated schema.
 *
 * @return generated fbs string
 */
std::string generate_fbs(gaia::common::gaia_id_t table_id);

/**
 * Generate FlatBuffers schema (fbs) from parsing result of a table.
 * Before the support of complex types, we can generate a table schema from its own own definition.
 * This method has no dependency on catalog or other table definitions.
 * Note: If we begin to supoort complex types, the method will need to be updated sigificantly.
 *
 * @param db_name database name
 * @param table_name table name
 * @param fields table fields parsing result bindings
 * @return fbs schema
 */
std::string generate_fbs(const std::string& db_name, const std::string& table_name, const ddl::field_def_list_t& fields);

/**
 * Generate binary FlatBuffers schema (bfbs) in base64 encoded string format.
 *
 * @param valid FlatBuffers schema
 * @return base64 encoded bfbs string
 */
std::vector<uint8_t> generate_bfbs(const std::string& fbs);

} // namespace catalog
} // namespace gaia
