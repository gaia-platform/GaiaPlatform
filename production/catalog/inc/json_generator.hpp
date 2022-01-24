/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "gaia_internal/catalog/catalog.hpp"

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
std::string generate_json(gaia::common::gaia_id_t table_id);

/**
 * Generate serializations (bin) in base64 encoded string format.
 *
 * @param fbs flatbuffers schema
 * @param json JSON data
 * @return base64 encoded serialization
 */
std::vector<uint8_t> generate_bin(const std::string& fbs, const std::string& json);

} // namespace catalog
} // namespace gaia
