/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include "access_control.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

// Each memory allocation will be prefixed by such a metadata block.
struct memory_allocation_metadata_t
{
    // Size of memory allocation block, including this metadata.
    uint64_t allocation_size;
};

// A stack_allocator_t's metadata information.
struct stack_allocator_metadata_t
{
    // Total allocation count.
    size_t count_allocations;

    // Offset where we can make the next allocation.
    address_offset_t next_allocation_offset;

    void clear()
    {
        count_allocations = 0;
        next_allocation_offset = c_invalid_offset;
    }
};

// The information about a stack_allocator_t allocation.
struct stack_allocator_allocation_t
{
    // The slot_id_t associated to this object.
    // This is an opaque value for us that we just store here,
    // for the serialization code to read later.
    slot_id_t slot_id;

    // The offset of the allocation.
    address_offset_t memory_offset;

    // The offset of the old allocation made for the previous copy of the object.
    address_offset_t old_memory_offset;

    void clear()
    {
        slot_id = 0;
        memory_offset = c_invalid_offset;
        old_memory_offset = c_invalid_offset;
    }
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
