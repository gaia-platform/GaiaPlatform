/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/common.hpp"

namespace gaia
{
namespace db
{

/**
 * This API assumes we're already in transaction scope.
 * Pass in non-null position_list to cache field positions.
 *
 * @param type_id of the paylod table type
 * @param payload1, payload2 payloads to compare
 * @return a list of positions of fields that diff
 */
common::field_position_list_t compute_payload_diff(
    common::gaia_type_t type_id,
    const uint8_t* payload1,
    const uint8_t* payload2);

} // namespace db
} // namespace gaia
