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
 * Generate JSON template data (json) for a catalog table.
 * The given table is the root type of the generated schema.
 *
 * @return generated JSON string
 */
string generate_json(gaia::common::gaia_id_t table_id);

/**
 * Generate a default data file (JSON) from parsing the definition of a table.
 *
 * @param fields table fields parsing result bindings
 * @return JSON file
 */
string generate_json(const ddl::field_def_list_t& fields);

/**
 * Generate serializations (bin) in base64 encoded string format.
 *
 * @param fbs flatbuffers schema
 * @param json JSON data
 * @return base64 encoded serialization
 */
string generate_bin(const string& fbs, const string& json);

/**
 * Retrieve the serialization template (bin) for a given table.
 *
 * @param table_id id of the table
 * @return bin
 */
vector<uint8_t> get_bin(gaia::common::gaia_id_t table_id);

} // namespace catalog
} // namespace gaia
