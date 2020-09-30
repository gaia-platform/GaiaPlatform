/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stdint.h>

#include "memory_types.hpp"
#include "access_control.hpp"

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

    // The size of the first allocation.
    // This is needed because the first allocation is not prefixed initially with its size.
    size_t first_allocation_size;

    // Offset where we can make the next allocation.
    address_offset_t next_allocation_offset;

    void clear()
    {
        count_allocations = 0;
        first_allocation_size = 0;
        next_allocation_offset = 0;
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
        memory_offset = 0;
        old_memory_offset = 0;
    }
};

}
}
}
