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

    // Sets memory_manager_t execution flags.
    void set_execution_flags(const execution_flags_t& execution_flags);

    // Tells the memory manager which memory area it should manage.
    //
    // All addresses will be offsets relative to the beginning of this block and will be represented as address_offset_t.
    error_code_t manage(
        uint8_t* memory_address,
        size_t memory_size);

    // Allocates a new block of memory.
    // This interface is meant to be used when the size of the block is known in advance,
    // such as in the case when we load the database objects from disk.
    error_code_t allocate(size_t memory_size, address_offset_t& allocated_memory_offset) const;

    // Once a transaction commits, calling this method will
    // add the stack allocator's unused memory to the list of free memory.
    error_code_t commit_stack_allocator(
        stack_allocator_t* stack_allocator) const;

private:

    struct memory_record_t
    {
        uint8_t* memory_address;
        size_t memory_size;

        memory_record_t()
        {
            clear();
        }

        void clear()
        {
            memory_address = nullptr;
            memory_size = 0;
        }
    };

    address_offset_t start_main_available_memory;

    std::list<memory_record_t> free_memory_list;
    mutable std::shared_mutex free_memory_list_lock;

    // Our execution flags.
    execution_flags_t m_execution_flags;

private:

    size_t get_main_memory_available_size(bool include_system_reserved_size) const;

    bool is_main_memory_exhausted(
        address_offset_t start_memory_offset,
        address_offset_t end_memory_offset,
        bool include_system_reserved_size) const;

    bool is_main_memory_exhausted(
        address_offset_t start_memory_offset,
        address_offset_t end_memory_offset,
        bool include_system_reserved_size,
        size_t& available_size) const;

    // Given an allocation offset, set up the allocation metadata and returns the offset past it.
    address_offset_t process_allocation(address_offset_t allocation_offset, size_t size_to_allocate) const;

    // Attempt to allocate from our main memory block.
    address_offset_t allocate_from_main_memory(size_t size_to_allocate) const;

    // Attempt to allocate from one of the already allocated and freed memory blocks.
    address_offset_t allocate_from_freed_memory(size_t size_to_allocate) const;

    void output_debugging_information(const std::string& context_description) const;

    void output_list_content(memory_record_t list_head) const;
};

}
}
}
