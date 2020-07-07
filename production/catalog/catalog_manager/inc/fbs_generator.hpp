/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_catalog.hpp"
#include "catalog_gaia_generated.h"

namespace gaia {
namespace catalog {

/**
 * Generate FlatBuffers schema (fbs) from parsing result of a table.
 * Before the support of complex types, we can generate a table schema from its own own definition.
 * This method has no dependency on catalog or other table definitions.
 * Note: If we begin to supoort complex types, the method will need to be updated sigificantly.
 *
 * @param table_name table name
 * @param fields table fields parsing result bindings
 * @return fbs schema
 */
string generate_fbs(const string &table_name, const vector<unique_ptr<ddl::field_definition_t>> &fields);

/**
 * Generate binary FlatBuffers schema (bfbs) in base64 encoded string format.
 *
 * @param valid FlatBuffers schema
 * @return base64 encoded bfbs string
 */
string generate_bfbs(const string &fbs);

/**
 * Convert the DDL parser binding data type enum to catalog record fbs definition enum.
 *
 * @param DDL data type
 * @return Gaia catalog data type
 */
gaia_data_type to_gaia_data_type(ddl::data_type_t data_type);

} // namespace catalog
} // namespace gaia
