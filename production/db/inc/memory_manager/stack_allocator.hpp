/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "base_memory_manager.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

class stack_allocator_t : public base_memory_manager_t
{
public:
    stack_allocator_t();

    // Initialize the stack_allocator_t with a specific memory buffer from which to allocate memory.
    // The start of the buffer is specified as an offset from a base address.
    void initialize(
        uint8_t* base_memory_address,
        address_offset_t memory_offset,
        size_t memory_size);

    // Load a specific memory buffer from which memory has already been allocated.
    // This method can be used to read the allocations made by another stack allocator instance.
    // The start of the buffer is specified as an offset from a base address.
    void load(
        uint8_t* base_memory_address,
        address_offset_t memory_offset,
        size_t memory_size);

    // Allocate a new memory block that will be designated by the provided slot id.
    // The old memory offset of the slot id is also provided, for later garbage collection.
    // A memory size of 0 indicates a deletion, but for clarity, use deallocate() method instead.
    address_offset_t allocate(
        slot_id_t slot_id,
        address_offset_t old_slot_offset,
        size_t memory_size) const;

    // Memory deallocation method.
    void deallocate(slot_id_t slot_id, address_offset_t slot_offset) const;

    // Deallocate all allocations made after the first N. This allows a transaction to quickly rollback changes.
    void deallocate(size_t count_allocations_to_keep) const;

    // Returns a pointer to the metadata structure, for direct access.
    stack_allocator_metadata_t* get_metadata() const;

    // Returns the total number of allocations performed so far (except those already rolled back).
    size_t get_allocation_count() const;

    // Returns a pointer to the N-th allocation record.
    // Allocation numbers start at 1.
    stack_allocator_allocation_t* get_allocation_record(size_t allocation_number) const;

    // Calculate the offset where we can make the next allocation.
    address_offset_t calculate_next_allocation_offset() const;

    // Mark the allocator as freed.
    void mark_as_freed();

    // Check whether the allocator has already been freed.
    bool has_been_freed();

    // Public, so it can get called by the memory manager.
    void output_debugging_information(const std::string& context_description) const;

private:
    // A pointer to our metadata information, stored at the end of the memory block that we manage.
    stack_allocator_metadata_t* m_metadata;

    // For protecting against double freeing.
    bool m_has_been_freed;

private:
    // Initialize the stack_allocator_t with a specific memory buffer from which to allocate memory.
    // The start of the buffer is specified as an offset from a base address.
    void initialize_internal(
        uint8_t* base_memory_address,
        address_offset_t memory_offset,
        size_t memory_size,
        bool initialize_memory);
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
