/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <iostream>

#include "base_memory_manager.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

class stack_allocator_t : public base_memory_manager_t
{
    friend class memory_manager_t;

public:

    stack_allocator_t();

    // Sets stack_allocator_t execution flags.
    void set_execution_flags(const execution_flags_t& execution_flags);

    // Initialize the stack_allocator_t with a specific memory buffer from which to allocate memory.
    // The start of the buffer is specified as an offset from a base address.
    // This is also only callable by a memory_manager_t.
    error_code_t initialize(uint8_t* base_memory_address, address_offset_t memory_offset, size_t memory_size);

    // Allocate a new memory block that will be designated by the provided slot id.
    // The old memory offset of the slot id is also provided, for later garbage collection.
    // A memory size of 0 indicates a deletion, but for clarity, use deallocate() method instead.
    error_code_t allocate(
        slot_id_t slot_id,
        address_offset_t old_slot_offset,
        size_t memory_size,
        address_offset_t& allocated_memory_offset) const;

    // Memory deallocation method.
    error_code_t deallocate(slot_id_t slot_id, address_offset_t slot_offset) const;

    // Deallocate all allocations made after the first N. This allows a transaction to quickly rollback changes.
    error_code_t deallocate(size_t count_allocations_to_keep) const;

    // Returns a pointer to the metadata structure, for direct access.
    stack_allocator_metadata_t* get_metadata() const;

    // Returns the total number of allocations performed so far (except those already rolled back).
    size_t get_allocation_count() const;

    // Returns a pointer to the N-th allocation record.
    // Allocation numbers start at 1.
    stack_allocator_allocation_t* get_allocation_record(size_t allocation_number) const;

    // Find the offset where we can make the next allocation.
    address_offset_t get_next_allocation_offset() const;

private:

    // A pointer to our metadata information, stored in the same memory that we manage.
    // Unlike the memory manager, which stores its metadata at the start of the buffer,
    // the stack allocator stores it at the end.
    stack_allocator_metadata_t* m_metadata;

    // Our execution flags.
    execution_flags_t m_execution_flags;

private:

    void output_debugging_information(const std::string& context_description) const;
};

}
}
}
