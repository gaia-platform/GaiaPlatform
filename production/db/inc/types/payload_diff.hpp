/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "field_list.hpp"
#include "types.hpp"

namespace gaia
{
namespace db
{
namespace types
{

field_list_t compute_payload_diff(gaia_id_t type_id, const uint8_t* payload1, const uint8_t* payload2);

shared_ptr<vector<field_position_t>> compute_payload_position_diff(gaia_id_t type_id, const uint8_t* payload1, const uint8_t* payload2);

}
}
}
