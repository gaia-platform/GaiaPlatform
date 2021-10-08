/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sys/mman.h>

#include "db_shared_data.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

inline chunk_offset_t chunk_from_offset(gaia_offset_t offset)
{
    // A chunk offset is just the 16 high bits of a 32-bit offset.
    static_assert(
        sizeof(gaia_offset_t) == sizeof(uint32_t)
        && sizeof(chunk_offset_t) == sizeof(uint16_t));
    return static_cast<chunk_offset_t>(offset >> 16);
}

inline slot_offset_t slot_from_offset(gaia_offset_t offset)
{
    // A slot offset is just the 16 low bits of a 32-bit offset.
    static_assert(
        sizeof(gaia_offset_t) == sizeof(uint32_t)
        && sizeof(slot_offset_t) == sizeof(uint16_t));
    // First mask out the 16 high bits (for correctness), then truncate.
    uint32_t mask = (1UL << 16) - 1;
    return static_cast<slot_offset_t>(offset & mask);
}

inline gaia_offset_t offset_from_chunk_and_slot(
    chunk_offset_t chunk_offset, slot_offset_t slot_offset)
{
    // A chunk offset is just the 16 high bits of a 32-bit offset,
    // and a slot offset is just the 16 low bits.
    static_assert(
        sizeof(gaia_offset_t) == sizeof(uint32_t)
        && sizeof(chunk_offset_t) == sizeof(uint16_t)
        && sizeof(slot_offset_t) == sizeof(uint16_t));
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
    ASSERT_INVARIANT(offset_ptr % c_slot_size_in_bytes == 0, "Expected object pointer to be aligned to 64 bytes!");

    uintptr_t page_ptr = c_page_size_in_bytes * (offset_ptr / c_page_size_in_bytes);
    return reinterpret_cast<void*>(page_ptr);
}

// Converts a slot offset to its bitmap index.
inline size_t slot_to_bit_index(slot_offset_t slot_offset)
{
    ASSERT_PRECONDITION(
        slot_offset >= c_first_slot_offset && slot_offset <= c_last_slot_offset,
        "Slot offset passed to is_slot_allocated() is out of bounds!");
    return slot_offset - c_first_slot_offset;
}

// Converts a slot offset to its index within a bitmap word.
inline size_t slot_to_bit_index_in_word(slot_offset_t slot_offset)
{
    size_t bit_index = slot_to_bit_index(slot_offset);
    return bit_index % c_uint64_bit_count;
}

// Converts a slot offset to the index of its bitmap word.
// The index of its data page within its chunk corresponds to the index of this
// bitmap word.
inline size_t slot_to_page_index(slot_offset_t slot_offset)
{
    size_t bit_index = slot_to_bit_index(slot_offset);
    return bit_index / c_uint64_bit_count;
}

inline slot_offset_t page_index_to_first_slot_in_page(size_t page_index)
{
    return (page_index * c_uint64_bit_count) + c_first_slot_offset;
}

// This helper is only used for DEBUG allocation mode, where we allocate all
// objects at page granularity and write-protect allocated pages after all
// updates to the allocated object are complete.
inline void write_protect_allocation_page_for_offset(gaia_offset_t offset)
{
    // Offset must be aligned to a page in debug mode.
    ASSERT_INVARIANT(
        ((offset * c_slot_size_in_bytes) % c_page_size_in_bytes) == 0,
        "Allocations must be page-aligned in debug mode!");
    void* offset_page = page_address_from_offset(offset);
    if (-1 == ::mprotect(offset_page, c_page_size_in_bytes, PROT_READ))
    {
        common::throw_system_error("mprotect(PROT_READ) failed!");
    }
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
