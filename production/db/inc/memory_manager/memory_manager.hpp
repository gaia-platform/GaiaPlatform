/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

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

    // Sets CMemoryManager execution flags.
    void set_execution_flags(const execution_flags_t& execution_flags);

    // Tells the memory manager which memory area it should manage.
    //
    // main_memory_system_reserved_size specifies a minimum size below which the available memory will not be allowed to drop.
    // Once this limit is reached, it can only be used by additional metadata allocations.
    // If exhausted, we're out of memory.
    //
    // If 'initialize' is set to true, the memory is considered to be uninitialized;
    // if not, then the manager will deal with it as if it was already initialized by another memory manager
    // - this means that parts of it are already allocated and in use.
    //
    // All addresses will be offsets relative to the beginning of this block, represented as ADDRESS_OFFSET.
    error_code_t manage(
        uint8_t* memory_address,
        size_t memory_size,
        size_t main_memory_system_reserved_size,
        bool initialize = true);

    // Allocates a new block of memory.
    // This interface is meant to be used when the size of the block is known in advance,
    // such as in the case when we load the database objects from disk.
    error_code_t allocate(size_t memory_size, address_offset_t& allocated_memory_offset) const;

    // Creates a StackAllocator object that will be used with transactions.
    // This interface will be used to allocate memory during regular database operation.
    error_code_t create_stack_allocator(size_t memory_size, stack_allocator_t*& stack_allocator) const;

    // Once a transaction commits, calling this method will achieve several goals:
    // (i) it will insert the block into a linked list of allocations
    // that is sorted by the associated serialization number.
    // (ii) it will add all old memory locations to the list of free memory.
    error_code_t commit_stack_allocator(
        stack_allocator_t* stack_allocator,
        serialization_number_t serialization_number) const;

    // This method should be called by the database engine serialization code.
    // This method exposes the list of allocations that is yet to be serialized to disk.
    error_code_t get_unserialized_allocations_list_head(memory_list_node_t*& list_head) const;

    // This method should be called by the database engine serialization code.
    // This method updates the next unserialized allocation at the end of a serialization.
    // We can now pack all serialized allocations before updating our list head.
    //
    // This method is not thread-safe! It assumes it is called from a single thread.
    // The method is thread-safe only with respect to the other memory manager operations.
    error_code_t update_unserialized_allocations_list_head(
        address_offset_t next_unserialized_allocation_record_offset) const;

    private:

    // All data tracked by the memory manager can be accessed from here.
    // The metadata information structure is stored at the beginning of the managed memory, 
    // but its lists are allocated at the end of it, so they will grow towards the beginning.
    // Also, because the metadata structure is located at offset 0, that offset can be considered to be invalid.
    struct metadata_t
    {
        address_offset_t start_main_available_memory;
        address_offset_t lowest_metadata_memory_use;

        memory_record_t free_memory_list_head;
        memory_record_t reclaimed_records_list_head;
        memory_record_t unserialized_allocations_list_head;

        void Clear(address_offset_t low, address_offset_t high)
        {
            start_main_available_memory = low;
            lowest_metadata_memory_use = high;

            free_memory_list_head.clear();
            reclaimed_records_list_head.clear();
            unserialized_allocations_list_head.clear();
        }
    };

    private:

    // A pointer to our metadata information, stored in the same memory that we manage.
    metadata_t* m_metadata;

    // There should be a single master manager called by the database engine.
    // Some functionality is restricted to this instance.
    bool m_is_master_manager;

    // See Manage() comment.
    size_t m_main_memory_system_reserved_size;

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

    // Get a memory record.
    memory_record_t* get_memory_record() const;

    // Attempt to allocate a new record from our main memory block.
    memory_record_t* get_new_memory_record() const;

    // Attempt to allocate from one of the previously allocated memory records.
    memory_record_t* get_reclaimed_memory_record() const;

    // Insert free memory record in its proper place in the list of free memory records.
    void insert_free_memory_record(memory_record_t* free_memory_record) const;

    // Insert memory record in the list of reclaimed memory records.
    void insert_reclaimed_memory_record(memory_record_t* reclaimed_memory_record) const;

    // Insert unserialized allocations record in the list of unserialized allocations.
    void insert_unserialized_allocations_record(memory_record_t* unserialized_allocations_record) const;

    // Mark memory as free by setting up a free memory record to track it.
    memory_record_t* get_free_memory_record(address_offset_t memory_offset, size_t memory_size) const;

    // Process a list of records by either adding its elements to the list of free memory records
    // or by reclaiming them.
    // This is only meant to be used by the following two methods - call them instead of this.
    void process_free_memory_records(memory_record_t** free_memory_records, size_t size, bool mark_as_free) const;

    // Insert records in the list of free memory records.
    void insert_free_memory_records(memory_record_t** free_memory_records, size_t size) const;

    // Insert records in the list of reclaimed memory records.
    void reclaim_records(memory_record_t** free_memory_records, size_t size) const;

    // Track the StackAllocator metadata for future serialization.
    error_code_t track_stack_allocator_metadata_for_serialization(
        stack_allocator_metadata_t* stack_allocator_metadata) const;

    void output_debugging_information(const string& context_description) const;

    void output_list_content(memory_record_t list_head) const;
};

}
}
}
