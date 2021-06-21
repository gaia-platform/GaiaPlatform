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
gaia::db::locators_t* get_locators();

// Returns a pointer to a mapping of the "counters" shared memory segment.
gaia::db::counters_t* get_counters();

// Returns a pointer to a mapping of the "data" shared memory segment.
gaia::db::data_t* get_data();

// Returns a pointer to a mapping of the "id_index" shared memory segment.
gaia::db::id_index_t* get_id_index();

// Allocate an object from the "data" shared memory segment.
gaia::db::memory_manager::address_offset_t allocate_object(
    gaia_locator_t locator,
    size_t size);

// Returns a pointer to the indexes.
gaia::db::index::indexes_t* get_indexes();

} // namespace db
} // namespace gaia
