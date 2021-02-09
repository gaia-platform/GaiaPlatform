/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "db_internal_types.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{

// Returns a pointer to a mapping of the "locators" shared memory segment.
gaia::db::shared_locators_t* get_shared_locators();

// Returns a pointer to a mapping of the "counters" shared memory segment.
gaia::db::shared_counters_t* get_shared_counters();

// Returns a pointer to a mapping of the "data" shared memory segment.
gaia::db::shared_data_t* get_shared_data();

// Returns a pointer to a mapping of the "id_index" shared memory segment.
gaia::db::shared_id_index_t* get_shared_id_index();

// Allocate an object from the "data" shared memory segment.
gaia::db::memory_manager::address_offset_t allocate_object(
    gaia_locator_t locator,
    size_t size);

} // namespace db
} // namespace gaia
