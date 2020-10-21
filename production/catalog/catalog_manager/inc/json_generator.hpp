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
 * Generate a default data file (JSON) from parsing the definition of a table.
 *
 * @param db_name database name
 * @param table_name table name
 * @param fields table fields parsing result bindings
 * @return JSON file
 */
string generate_json(const string& db_name, const string& table_name, const ddl::field_def_list_t& fields);

/**
 * Generate serializations (bin) in base64 encoded string format.
 *
 * @param fbs flatbuffers schema
 * @param json JSON data
 * @return base64 encoded serialization
 */
string generate_bin(const string& fbs, const string& json);

} // namespace catalog
} // namespace gaia
