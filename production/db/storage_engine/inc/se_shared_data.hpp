/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "memory_types.hpp"
#include "se_types.hpp"

namespace gaia
{
namespace db
{

// Returns a pointer to a mapping of the "locators" shared memory segment.
gaia::db::locators* get_shared_locators();

// Returns a pointer to a mapping of the "data" shared memory segment.
gaia::db::data* get_shared_data();

// Allocate an object from the "data" shared memory segment.
gaia::db::memory_manager::address_offset_t allocate_object(
    gaia_locator_t locator,
    gaia::db::memory_manager::address_offset_t old_slot_offset,
    size_t size);

} // namespace db
} // namespace gaia
