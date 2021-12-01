/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include "gaia/db/db.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"
#include "gaia_internal/exceptions.hpp"

#include "chunk_manager.hpp"
#include "db_internal_types.hpp"
#include "db_shared_data.hpp"
#include "memory_manager.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{

inline common::gaia_id_t allocate_id()
{
    counters_t* counters = gaia::db::get_counters();
    return ++(counters->last_id);
}

inline common::gaia_type_t allocate_type()
{
    counters_t* counters = gaia::db::get_counters();
    return ++(counters->last_type_id);
}

inline gaia_txn_id_t allocate_txn_id()
{
    counters_t* counters = gaia::db::get_counters();
    return ++(counters->last_txn_id);
}

inline gaia_locator_t allocate_locator()
{
    counters_t* counters = gaia::db::get_counters();

    if (counters->last_locator >= c_max_locators)
    {
        throw system_object_limit_exceeded_internal();
    }

    return ++(counters->last_locator);
}

inline void update_locator(gaia_locator_t locator, gaia_offset_t offset)
{
    locators_t* locators = gaia::db::get_locators_for_allocator();

    (*locators)[locator] = offset;
}

inline bool locator_exists(gaia_locator_t locator)
{
    locators_t* locators = gaia::db::get_locators();
    counters_t* counters = gaia::db::get_counters();

    return (locator != c_invalid_gaia_locator)
        && (locator <= counters->last_locator)
        && ((*locators)[locator] != c_invalid_gaia_offset);
}

inline gaia_offset_t locator_to_offset(gaia_locator_t locator)
{
    locators_t* locators = gaia::db::get_locators();
    return locator_exists(locator)
        ? (*locators)[locator].load()
        : c_invalid_gaia_offset.value();
}

inline db_object_t* offset_to_ptr(gaia_offset_t offset)
{
    data_t* data = gaia::db::get_data();
    return (offset != c_invalid_gaia_offset)
        ? reinterpret_cast<db_object_t*>(&data->objects[offset])
        : nullptr;
}

inline db_object_t* locator_to_ptr(gaia_locator_t locator)
{
    return offset_to_ptr(locator_to_offset(locator));
}

// This is only meant for "fuzzy snapshots" of the current last_txn_id; there
// are no memory barriers.
inline gaia_txn_id_t get_last_txn_id()
{
    counters_t* counters = gaia::db::get_counters();
    return counters->last_txn_id.load();
}

inline void apply_log_to_locators(locators_t* locators, txn_log_t* log)
{
    for (size_t i = 0; i < log->record_count; ++i)
    {
        auto& record = log->log_records[i];
        (*locators)[record.locator] = record.new_offset;
    }
}

inline index::db_index_t id_to_index(common::gaia_id_t index_id)
{
    auto it = get_indexes()->find(index_id);

    return (it != get_indexes()->end()) ? it->second : nullptr;
}

// Allocate an object from the "data" shared memory segment.
// The `size` argument *does not* include the object header size!
inline void allocate_object(
    gaia_locator_t locator,
    size_t size)
{
    memory_manager::memory_manager_t* memory_manager = gaia::db::get_memory_manager();
    memory_manager::chunk_manager_t* chunk_manager = gaia::db::get_chunk_manager();

    // The allocation can fail either because there is no current chunk, or
    // because the current chunk is full.
    gaia_offset_t object_offset = chunk_manager->allocate(size + c_db_object_header_size);
    if (object_offset == c_invalid_gaia_offset)
    {
        if (chunk_manager->initialized())
        {
            // The current chunk is out of memory, so retire it and allocate a new chunk.
            // In case it is already empty, try to deallocate it after retiring it.

            // Get the session's chunk version for safe deallocation.
            memory_manager::chunk_version_t version = chunk_manager->get_version();
            // Now retire the chunk.
            chunk_manager->retire_chunk(version);
            // Release ownership of the chunk.
            chunk_manager->release();
        }

        // Allocate a new chunk.
        memory_manager::chunk_offset_t new_chunk_offset = memory_manager->allocate_chunk();
        if (new_chunk_offset == memory_manager::c_invalid_chunk_offset)
        {
            throw memory_allocation_error_internal();
        }

        // Initialize the new chunk.
        chunk_manager->initialize(new_chunk_offset);

        // Before we allocate, persist current chunk ID in txn log, for access
        // on the server in case we crash.
        gaia::db::get_mapped_log()->data()->current_chunk = new_chunk_offset;

        // Allocate from new chunk.
        object_offset = chunk_manager->allocate(size + c_db_object_header_size);
    }

    ASSERT_POSTCONDITION(
        object_offset != c_invalid_gaia_offset,
        "Allocation from chunk was not expected to fail!");

    // Update locator array to point to the new offset.
    update_locator(locator, object_offset);
}

} // namespace db
} // namespace gaia
