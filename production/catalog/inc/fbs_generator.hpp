////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <string>

#include "gaia_internal/catalog/catalog.hpp"

namespace gaia
{
namespace catalog
{

/**
 * Generate FlatBuffers schema (fbs) for a catalog table.
 * The given table is the root type of the generated schema.
 *
 * @param table_id The table id for which to generate the FlatBuffers schema
 * @param ignore_optional This is a workaround to prevent the generation of optional
 *        scalar values which, as of flatbuffers 2.0.0, are not supported when reading
 *        data from JSON. When generating a fbs meant for JSON template data this
 *        value should be true, false otherwise.
 *        See: https://github.com/google/flatbuffers/issues/6975
 * @return generated fbs string
 */
std::string generate_fbs(gaia::common::gaia_id_t table_id, bool ignore_optional = false);

/**
 * Generate binary FlatBuffers schema (bfbs) in base64 encoded string format.
 *
 * @param valid FlatBuffers schema
 * @return base64 encoded bfbs string
 */
std::vector<uint8_t> generate_bfbs(const std::string& fbs);

} // namespace catalog
} // namespace gaia
