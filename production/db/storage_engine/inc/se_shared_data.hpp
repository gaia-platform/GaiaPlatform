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
gaia::db::locators_t* get_shared_locators();

// Returns a pointer to a mapping of the "counters" shared memory segment.
gaia::db::shared_counters_t* get_shared_counters();

// Returns a pointer to a mapping of the "data" shared memory segment.
gaia::db::shared_data_t* get_shared_data();

// Returns a pointer to a mapping of the "id_index" shared memory segment.
gaia::db::shared_id_index_t* get_shared_id_index();

// Returns a pointer to a mapping of the "page_alloc_counts" shared memory segment.
gaia::db::page_alloc_counts_t* get_shared_page_alloc_counts();

} // namespace db
} // namespace gaia
