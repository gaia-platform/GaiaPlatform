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
    uint64_t allocationSize;
};

// Common structure of an in-memory linked list node
// that includes an AccessControl field for synchronization.
struct MemoryListNode
{
    ADDRESS_OFFSET next;
    AccessControl accessControl;

    void Clear()
    {
        next = 0;
        accessControl.Clear();
    }
};

// Generic structure meant for tracking various memory objects,
// such as freed memory blocks or StackAllocator metadata records.
struct MemoryRecord : MemoryListNode
{
    ADDRESS_OFFSET memoryOffset;
    size_t memorySize;

    void Clear()
    {
        MemoryListNode::Clear();

        memoryOffset = 0;
        memorySize = 0;
    }
};

// A StackAllocator's metadata information.
struct StackAllocatorMetadata
{
    // Total allocation count.
    size_t countAllocations;

    // The size of the first allocation.
    // This is needed because the first allocation is not prefixed initially with its size.
    size_t firstAllocationSize;

    // Offset where we can make the next allocation.
    ADDRESS_OFFSET nextAllocationOffset;

    // Serialization number associated with this allocator; this will be set late.
    SERIALIZATION_NUMBER serializationNumber;

    void Clear()
    {
        countAllocations = 0;
        firstAllocationSize = 0;
        nextAllocationOffset = 0;
        serializationNumber = 0;
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
    ADDRESS_OFFSET memoryOffset;

    // The offset of the old allocation made for the previous copy of the object.
    ADDRESS_OFFSET oldMemoryOffset;

    void Clear()
    {
        slotId = 0;
        memoryOffset = 0;
        oldMemoryOffset = 0;
    }
};

}
}
}

