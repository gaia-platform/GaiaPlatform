/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>
#include "gaia_common.hpp"
#include <vector>

using namespace gaia::common;

namespace gaia
{
namespace db
{

// Type definitions for fields.
typedef uint16_t field_position_t;

struct object {
    gaia_id_t id;
    gaia_type_t type;
    uint64_t num_references;
    uint64_t payload_size;
    char payload[0];
};
// Field position list.
typedef std::vector<field_position_t> field_position_list_t;

} // namespace db
} // namespace gaia
