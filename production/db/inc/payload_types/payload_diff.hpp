/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "field_list.hpp"

namespace gaia
{
namespace db
{
namespace payload_types
{

/**
 * This API assumes we're already in transaction scope.
 * Pass in non-null position_list to cache field positions.
 */
void compute_payload_diff(gaia_type_t type_id, const uint8_t* payload1, const uint8_t* payload2, field_position_list_t* position_list);

}
}
}
