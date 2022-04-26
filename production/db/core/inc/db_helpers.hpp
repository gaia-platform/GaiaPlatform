/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include "gaia/db/db.hpp"

#include "gaia_internal/common/debug_assert.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/exceptions.hpp"

#include "chunk_manager.hpp"
#include "db_hash_map.hpp"
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
    auto new_id = ++(counters->last_id);

    // This is an expensive check in a hot path.
    DEBUG_ASSERT_INVARIANT(
        new_id <= std::numeric_limits<common::gaia_id_t::value_type>::max(),
        "Gaia ID exceeds allowed range!");

    return static_cast<common::gaia_id_t>(new_id);
}

inline gaia_txn_id_t allocate_txn_id()
{
    counters_t* counters = gaia::db::get_counters();
    auto new_txn_id = ++(counters->last_txn_id);

    // This is an expensive check in a hot path.
    DEBUG_ASSERT_INVARIANT(
        new_txn_id < (1UL << transactions::txn_metadata_entry_t::c_txn_ts_bit_width),
        "Transaction ID exceeds allowed range!");

    return static_cast<gaia_txn_id_t>(new_txn_id);
}

inline gaia_locator_t allocate_locator(common::gaia_type_t type)
{
    counters_t* counters = gaia::db::get_counters();

    if (counters->last_locator >= c_max_locators)
    {
        throw system_object_limit_exceeded_internal();
    }

    auto new_locator_value = ++(counters->last_locator);
    auto new_locator = static_cast<gaia_locator_t>(new_locator_value);

    type_index_t* type_index = get_type_index();
    type_index->add_locator(type, new_locator);

    return new_locator;
}

inline void update_locator(gaia_locator_t locator, gaia_offset_t offset)
{
    locators_t* locators = gaia::db::get_locators_for_allocator();

    (*locators)[locator] = offset;
}

inline gaia_locator_t get_last_locator()
{
    counters_t* counters = gaia::db::get_counters();
    auto last_locator_value = counters->last_locator.load();

    // This is an expensive check in a hot path.
    DEBUG_ASSERT_INVARIANT(
        last_locator_value <= c_max_locators,
        "Largest locator value exceeds allowed range!");

    return static_cast<gaia_locator_t>(last_locator_value);
}

inline bool locator_exists(gaia_locator_t locator)
{
    locators_t* locators = gaia::db::get_locators();
    return (locator.is_valid())
        && (locator <= get_last_locator())
        && ((*locators)[locator] != c_invalid_gaia_offset);
}

// Returns true if ID was not already registered, false otherwise.
inline bool register_locator_for_id(
    common::gaia_id_t id, gaia_locator_t locator)
{
    return gaia::db::db_hash_map::insert(id, locator);
}

inline gaia_locator_t id_to_locator(common::gaia_id_t id)
{
    return id.is_valid() ? gaia::db::db_hash_map::find(id) : c_invalid_gaia_locator;
}

inline gaia_offset_t locator_to_offset(gaia_locator_t locator)
{
    locators_t* locators = gaia::db::get_locators();
    return locator_exists(locator)
        ? (*locators)[locator]
        : c_invalid_gaia_offset;
}

inline db_object_t* offset_to_ptr(gaia_offset_t offset)
{
    data_t* data = gaia::db::get_data();
    return (offset.is_valid())
        ? reinterpret_cast<db_object_t*>(&data->objects[offset])
        : nullptr;
}

inline db_object_t* locator_to_ptr(gaia_locator_t locator)
{
    return offset_to_ptr(locator_to_offset(locator));
}

inline db_object_t* id_to_ptr(common::gaia_id_t id)
{
    gaia_locator_t locator = id_to_locator(id);
    ASSERT_INVARIANT(
        locator_exists(locator),
        "An invalid locator was returned by id_to_locator()!");
    return locator_to_ptr(locator);
}

// This is only meant for "fuzzy snapshots" of the current last_txn_id; there
// are no memory barriers.
inline gaia_txn_id_t get_last_txn_id()
{
    counters_t* counters = gaia::db::get_counters();
    return static_cast<gaia_txn_id_t>(counters->last_txn_id);
}

inline void apply_log_to_locators(locators_t* locators, txn_log_t* txn_log, size_t starting_log_record_index = 0)
{
    for (size_t i = starting_log_record_index; i < txn_log->record_count; ++i)
    {
        auto log_record = &(txn_log->log_records[i]);
        (*locators)[log_record->locator] = log_record->new_offset;
    }
}

inline gaia::db::txn_log_t* get_txn_log_from_offset(log_offset_t offset)
{
    ASSERT_PRECONDITION(offset != gaia::db::c_invalid_log_offset, "Txn log offset is invalid!");
    gaia::db::logs_t* logs = gaia::db::get_logs();
    return &((*logs)[offset]);
}

inline void apply_log_from_offset(locators_t* locators, log_offset_t log_offset, size_t starting_log_record_index = 0)
{
    txn_log_t* txn_log = get_txn_log_from_offset(log_offset);
    apply_log_to_locators(locators, txn_log, starting_log_record_index);
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
    if (!object_offset.is_valid())
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
        if (!new_chunk_offset.is_valid())
        {
            throw memory_allocation_error_internal();
        }

        // Initialize the new chunk.
        chunk_manager->initialize(new_chunk_offset);

        // Allocate from new chunk.
        object_offset = chunk_manager->allocate(size + c_db_object_header_size);
    }

    ASSERT_POSTCONDITION(
        object_offset.is_valid(),
        "Allocation from chunk was not expected to fail!");

    // Update locator array to point to the new offset.
    update_locator(locator, object_offset);
}

inline bool acquire_txn_log_reference(log_offset_t log_offset, gaia_txn_id_t begin_ts)
{
    txn_log_t* txn_log = get_txn_log_from_offset(log_offset);
    return txn_log->acquire_reference(begin_ts);
}

inline void release_txn_log_reference(log_offset_t log_offset, gaia_txn_id_t begin_ts)
{
    txn_log_t* txn_log = get_txn_log_from_offset(log_offset);
    txn_log->release_reference(begin_ts);
}

} // namespace db
} // namespace gaia
