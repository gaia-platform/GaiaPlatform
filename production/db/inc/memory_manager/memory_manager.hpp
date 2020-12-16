/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <iostream>
#include <list>
#include <shared_mutex>

#include "base_memory_manager.hpp"
#include "stack_allocator.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

class memory_manager_t : public base_memory_manager_t
{
public:
    memory_manager_t();

    // Tells the memory manager which memory area it should manage.
    //
    // All addresses will be offsets relative to the beginning of this block
    // and will be represented as address_offset_t.
    void manage(
        uint8_t* memory_address,
        size_t memory_size);

    // Allocates a new block of memory.
    // This is used for the initial load of data in memory.
    // Each allocation will get prefixed by a metadata block.
    address_offset_t allocate(size_t memory_size);

    // Allocates a new block of memory.
    // This is used for allocating memory blocks to be used with stack allocators.
    // These allocations are NOT prefixed with a metadata block.
    address_offset_t allocate_raw(size_t memory_size);

    // Adds the stack allocator's unused memory and allocation records to the list of free memory.
    void free_stack_allocator(
        const std::unique_ptr<stack_allocator_t>& stack_allocator);

    // Adds object memory at the provided offset to the list of free memory.
    void free_old_offset(const address_offset_t offset);

private:
    // This structure is used for tracking information about a memory block.
    struct memory_record_t
    {
        address_offset_t memory_offset;
        size_t memory_size;

        memory_record_t(address_offset_t memory_offset, size_t memory_size)
        {
            this->memory_offset = memory_offset;
            this->memory_size = memory_size;
        }
    };

    // As we keep allocating memory, the remaining contiguous available memory block
    // will keep shrinking. We'll use this offset to track the start of the block.
    address_offset_t m_next_allocation_offset;

    // Free memory list.
    std::list<memory_record_t> m_free_memory_list;
    mutable std::shared_mutex m_free_memory_list_lock;

private:
    size_t get_main_memory_available_size() const;

    // Given an allocation offset, set up the allocation metadata and returns the offset past it.
    // If add_allocation_metadata is set to false, the memory allocation won't get prefixed by a metadata block.
    address_offset_t process_allocation(
        address_offset_t allocation_offset,
        size_t size_to_allocate,
        bool add_allocation_metadata) const;

    // Allocates a new block of memory.
    address_offset_t allocate_internal(
        size_t memory_size,
        bool add_allocation_metadata);

    // Attempt to allocate from our main memory block.
    address_offset_t allocate_from_main_memory(size_t size_to_allocate, bool add_allocation_metadata);

    // Attempt to allocate from one of the already allocated and freed memory blocks.
    address_offset_t allocate_from_freed_memory(size_t size_to_allocate, bool add_allocation_metadata);

    void output_debugging_information(const std::string& context_description) const;

    void output_free_memory_list() const;
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
