/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include "gaia_common.hpp"

namespace gaia
{
namespace db
{

using namespace common;

// This was factored out of gaia_ptr.hpp because the server needs to know
// the object format but doesn't need any gaia_ptr functionality.
struct gaia_se_object_t
{
    gaia_id_t id;
    gaia_type_t type;
    // The Flatbuffer size limit is 2GB (signed 32-bit). With a 16-bit payload size,
    // the limit is 65,536 bytes. This total size of the payload will be the
    // serialized flatbuffer size plus the num_references * sizeof(gaia_id_t).
    uint16_t payload_size;
    uint16_t num_references;
    char payload[0];
};

} // namespace db
} // namespace gaia
