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
struct memory_allocation_metadata_t
{
    // Size of memory allocation block, including this metadata.
    uint64_t allocation_size;
};

// Common structure of an in-memory linked list node
// that includes an access_control_t field for synchronization.
struct memory_list_node_t
{
    address_offset_t next;
    access_control_t access_control;

    void clear()
    {
        next = 0;
        access_control.clear();
    }
};

// Generic structure meant for tracking various memory objects,
// such as freed memory blocks or stack_allocator_t metadata records.
struct memory_record_t : memory_list_node_t
{
    address_offset_t memory_offset;
    size_t memory_size;

    void clear()
    {
        memory_list_node_t::clear();

        memory_offset = 0;
        memory_size = 0;
    }
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

    // Serialization number associated with this allocator; this will be set late.
    serialization_number_t serialization_number;

    void clear()
    {
        count_allocations = 0;
        first_allocation_size = 0;
        next_allocation_offset = 0;
        serialization_number = 0;
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
