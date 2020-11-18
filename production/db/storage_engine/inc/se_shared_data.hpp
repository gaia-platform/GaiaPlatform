/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "se_types.hpp"

namespace gaia
{
namespace db
{

// Returns a pointer to a mapping of the "locators" shared memory segment.
gaia::db::locators* get_shared_locators();

// Returns a pointer to a mapping of the "data" shared memory segment.
gaia::db::data* get_shared_data();

} // namespace db
} // namespace gaia
