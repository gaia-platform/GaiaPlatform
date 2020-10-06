/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <vector>

#include "gaia_common.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{

// Storage engine internal object type.
struct gaia_se_object_t {
    gaia_id_t id;
    gaia_type_t type;
    uint64_t num_references;
    uint64_t payload_size;
    char payload[0];
};

} // namespace db
} // namespace gaia
