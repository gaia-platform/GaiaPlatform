/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>
#include "gaia_common.hpp"

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
    size_t num_references;
    size_t payload_size;
    char payload[0];
};

} // namespace db
} // namespace gaia
