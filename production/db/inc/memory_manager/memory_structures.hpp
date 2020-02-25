/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "types.hpp"
#include "access_control.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

// Each memory allocation will be prefixed by such a metadata block.
struct MemoryAllocationMetadata
{
    // Size of memory allocation block, including this metadata.
    uint64_t allocation_size;
};

// Common structure of an in-memory linked list node
// that includes an AccessControl field for synchronization.
struct MemoryListNode
{
    ADDRESS_OFFSET next;
    AccessControl accessControl;

    void clear()
    {
        next = 0;
        accessControl.clear();
    }
};

// Generic structure meant for tracking various memory objects,
// such as freed memory blocks or StackAllocator metadata records.
struct MemoryRecord : MemoryListNode
{
    ADDRESS_OFFSET memory_offset;
    size_t memory_size;

    void clear()
    {
        MemoryListNode::clear();

        memory_offset = 0;
        memory_size = 0;
    }
};

// A StackAllocator's metadata information.
struct StackAllocatorMetadata
{
    // Total allocation count.
    size_t count_allocations;

    // The size of the first allocation.
    // This is needed because the first allocation is not prefixed initially with its size.
    size_t first_allocation_size;

    // Offset where we can make the next allocation.
    ADDRESS_OFFSET next_allocation_offset;

    // Serialization number associated with this allocator; this will be set late.
    SERIALIZATION_NUMBER serialization_number;

    void clear()
    {
        count_allocations = 0;
        first_allocation_size = 0;
        next_allocation_offset = 0;
        serialization_number = 0;
    }
};

// The information about a StackAllocator allocation.
struct StackAllocatorAllocation
{
    // The slotId associated to this object.
    // This is an opaque value for us that we just store here,
    // for the serialization code to read later.
    SLOT_ID slotId;

    // The offset of the allocation.
    ADDRESS_OFFSET memory_offset;

    // The offset of the old allocation made for the previous copy of the object.
    ADDRESS_OFFSET old_memory_offset;

    void clear()
    {
        slotId = 0;
        memory_offset = 0;
        old_memory_offset = 0;
    }
};

}
}
}

