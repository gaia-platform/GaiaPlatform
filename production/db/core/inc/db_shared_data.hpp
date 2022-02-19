/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "db_internal_types.hpp"
#include "mapped_data.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{

// We forward-declare these definitions instead of including their header files,
// to avoid circular dependencies.
namespace memory_manager
{
class memory_manager_t;
class chunk_manager_t;
} // namespace memory_manager

// Returns a pointer to a mapping of the "locators" shared memory segment.
gaia::db::locators_t* get_locators();

// Returns a pointer to a mapping of the "locators" shared memory segment for the allocator.
gaia::db::locators_t* get_locators_for_allocator();

// Returns a pointer to a mapping of the "counters" shared memory segment.
gaia::db::counters_t* get_counters();

// Returns a pointer to a mapping of the "data" shared memory segment.
gaia::db::data_t* get_data();

// Returns a pointer to a mapping of the "logs" shared memory segment.
gaia::db::logs_t* get_logs();

// Returns a pointer to a mapping of the "id_index" shared memory segment.
gaia::db::id_index_t* get_id_index();

// Get the current txn id.
gaia::db::gaia_txn_id_t get_current_txn_id();

// Returns a pointer to the indexes.
gaia::db::index::indexes_t* get_indexes();

// Gets the memory manager instance for the current thread or process.
gaia::db::memory_manager::memory_manager_t* get_memory_manager();

// Gets the chunk manager instance for the current thread or process.
gaia::db::memory_manager::chunk_manager_t* get_chunk_manager();

// Gets the mapped transaction log for the current session thread.
gaia::db::mapped_log_t* get_mapped_log();

// Gets the transaction log for the current session thread.
gaia::db::txn_log_t* get_txn_log();

} // namespace db
} // namespace gaia
