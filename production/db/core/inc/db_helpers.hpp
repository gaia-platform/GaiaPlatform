/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/exception.hpp"

#include "gaia_internal/common/memory_allocation_error.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

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
    common::gaia_id_t id = __sync_add_and_fetch(&counters->last_id, 1);
    return id;
}

inline common::gaia_type_t allocate_type()
{
    counters_t* counters = gaia::db::get_counters();
    common::gaia_type_t type = __sync_add_and_fetch(&counters->last_type_id, 1);
    return type;
}

inline gaia_txn_id_t allocate_txn_id()
{
    counters_t* counters = gaia::db::get_counters();
    gaia_txn_id_t txn_id = __sync_add_and_fetch(&counters->last_txn_id, 1);
    return txn_id;
}

inline gaia_locator_t allocate_locator()
{
    counters_t* counters = gaia::db::get_counters();

    // We need an acquire barrier before reading `last_locator`. We can
    // change this full barrier to an acquire barrier when we change to proper
    // C++ atomic types.
    __sync_synchronize();

    if (counters->last_locator >= c_max_locators)
    {
        throw system_object_limit_exceeded();
    }

    return __sync_add_and_fetch(&counters->last_locator, 1);
}

inline constexpr size_t get_gaia_alignment_unit()
{
    return gaia::db::memory_manager::c_allocation_alignment;
}

inline gaia_offset_t get_gaia_offset(gaia::db::memory_manager::address_offset_t offset)
{
    if (offset == gaia::db::memory_manager::c_invalid_address_offset)
    {
        return c_invalid_gaia_offset;
    }

    return offset / get_gaia_alignment_unit();
}

inline gaia::db::memory_manager::address_offset_t get_address_offset(gaia_offset_t offset)
{
    if (offset == c_invalid_gaia_offset)
    {
        return gaia::db::memory_manager::c_invalid_address_offset;
    }

    return offset * get_gaia_alignment_unit();
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

    // We need an acquire barrier before reading `last_locator`. We can
    // change this full barrier to an acquire barrier when we change to proper
    // C++ atomic types.
    __sync_synchronize();

    return (locator != c_invalid_gaia_locator)
        && (locator <= counters->last_locator)
        && ((*locators)[locator] != c_invalid_gaia_offset);
}

inline gaia_offset_t locator_to_offset(gaia_locator_t locator)
{
    locators_t* locators = gaia::db::get_locators();
    return locator_exists(locator)
        ? (*locators)[locator].load()
        : c_invalid_gaia_offset;
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
    return counters->last_txn_id;
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

inline memory_manager::chunk_offset_t chunk_from_offset(gaia_offset_t offset)
{
    // A chunk offset is just the 16 high bits of a 32-bit offset.
    static_assert(sizeof(gaia_offset_t) == 4 && sizeof(memory_manager::chunk_offset_t) == 2);
    return static_cast<memory_manager::chunk_offset_t>(offset >> 16);
}

inline memory_manager::slot_offset_t slot_from_offset(gaia_offset_t offset)
{
    // A slot offset is just the 16 low bits of a 32-bit offset.
    static_assert(sizeof(gaia_offset_t) == 4 && sizeof(memory_manager::slot_offset_t) == 2);
    // First mask out the 16 high bits (for correctness), then truncate.
    uint32_t mask = (1UL << 16) - 1;
    return static_cast<memory_manager::slot_offset_t>(offset & mask);
}

inline gaia_offset_t offset_from_chunk_and_slot(
    memory_manager::chunk_offset_t chunk_offset, memory_manager::slot_offset_t slot_offset)
{
    // A chunk offset is just the 16 high bits of a 32-bit offset,
    // and a slot offset is just the 16 low bits.
    static_assert(
        sizeof(gaia_offset_t) == 4
        && sizeof(memory_manager::chunk_offset_t) == 2
        && sizeof(memory_manager::slot_offset_t) == 2);
    return (chunk_offset << 16) | slot_offset;
}

inline void* page_address_from_offset(gaia_offset_t offset)
{
    ASSERT_PRECONDITION(
        offset != c_invalid_gaia_offset,
        "Cannot call page_address_from_offset() on an invalid offset!");

    data_t* data = gaia::db::get_data();
    uintptr_t offset_ptr = reinterpret_cast<uintptr_t>(&data->objects[offset]);

    // A pointer to db_object_t must be 64-byte-aligned.
    ASSERT_INVARIANT(offset_ptr % 64 == 0, "Expected object pointer to be aligned to 64 bytes!");

    uintptr_t page_ptr = memory_manager::c_page_size_bytes * (offset_ptr / memory_manager::c_page_size_bytes);
    return reinterpret_cast<void*>(page_ptr);
}

inline size_t slot_to_bit_index(memory_manager::slot_offset_t slot_offset)
{
    ASSERT_PRECONDITION(
        slot_offset >= memory_manager::c_first_slot_offset && slot_offset <= memory_manager::c_last_slot_offset,
        "Slot offset passed to is_slot_allocated() is out of bounds!");
    return slot_offset - memory_manager::c_first_slot_offset;
}

// Converts a slot offset to its bit index within a single word.
inline size_t slot_to_word_index(memory_manager::slot_offset_t slot_offset)
{
    size_t bit_index = slot_to_bit_index(slot_offset);
    return bit_index % memory_manager::c_uint64_bit_count;
}

inline size_t slot_to_page_index(memory_manager::slot_offset_t slot_offset)
{
    size_t bit_index = slot_to_word_index(slot_offset);
    // A page of data corresponds to a single word in the bitmaps.
    return bit_index / memory_manager::c_uint64_bit_count;
}

inline memory_manager::slot_offset_t page_index_to_first_slot_in_page(size_t page_index)
{
    return (page_index * memory_manager::c_uint64_bit_count) + memory_manager::c_first_slot_offset;
}

// Allocate an object from the "data" shared memory segment.
// The `size` argument *does not* include the object header size!
inline void allocate_object(
    gaia_locator_t locator,
    size_t size)
{
    memory_manager::memory_manager_t* memory_manager = gaia::db::get_memory_manager();
    memory_manager::chunk_manager_t* chunk_manager = gaia::db::get_chunk_manager();

    // REVIEW: This assert should really be in chunk_manager_t.allocate(), but
    // that class doesn't have a reference to the memory manager, so it can't
    // query global chunk metadata.
    // If we have a current chunk, it must be in the IN_USE state.
    memory_manager::chunk_offset_t current_chunk_offset = chunk_manager->chunk_offset();
    if (current_chunk_offset != memory_manager::c_invalid_chunk_offset)
    {
        memory_manager::chunk_state_t chunk_state = chunk_manager->get_state();
        ASSERT_PRECONDITION(
            chunk_state == memory_manager::chunk_state_t::in_use,
            "Cannot allocate from a chunk that is not in the IN_USE state!");
    }

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
        }

        // Allocate a new chunk.
        memory_manager::chunk_offset_t new_chunk_offset = memory_manager->allocate_chunk();
        if (new_chunk_offset == memory_manager::c_invalid_chunk_offset)
        {
            throw memory_allocation_error("Memory manager ran out of memory during call to allocate_chunk().");
        }

        // Initialize the new chunk.
        chunk_manager->initialize(new_chunk_offset);

        // Before we allocate, persist current chunk ID in txn log, for access on the server in case we crash.
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
