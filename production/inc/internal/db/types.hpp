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

// Storage engine internal object type.
struct gaia_se_object_t {
    gaia_id_t id;
    gaia_container_id_t type;
    uint64_t num_references;
    uint64_t payload_size;
    char payload[0];
};

} // namespace db
} // namespace gaia
