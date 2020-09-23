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
    gaia_type_t type;
    // The Flatbuffer size limit is 2GB (signed 32-bit). Total size of the payload will
    // be the serialized flatbuffer size plus the num_references * sizeof(gaia_id_t).
    uint32_t payload_size;
    uint16_t num_references;
    char payload[0];
};

} // namespace db
} // namespace gaia
