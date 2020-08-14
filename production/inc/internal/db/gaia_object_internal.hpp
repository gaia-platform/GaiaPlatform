/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include "gaia_common.hpp"

namespace gaia {
namespace db {

using namespace common;

// This was factored out of gaia_ptr.hpp because the server needs to know
// the object format but doesn't need any gaia_ptr functionality.
struct object {
    gaia_id_t id;
    gaia_type_t type;
    size_t num_references;
    size_t payload_size;
    char payload[0];
};

}  // namespace db
}  // namespace gaia
