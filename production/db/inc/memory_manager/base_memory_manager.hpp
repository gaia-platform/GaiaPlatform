/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "types.hpp"
#include "error_codes.hpp"
#include "structures.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

class base_memory_manager
{
    public:

    static const size_t c_memory_alignment = sizeof(uint64_t);

    public:

    base_memory_manager();

    // Sanity checks.
    bool validate_address_alignment(const uint8_t* const pMemoryAddress) const;
    bool validate_offset_alignment(ADDRESS_OFFSET memoryOffset) const;
    bool validate_size_alignment(size_t memorySize) const;

    error_code validate_address(const uint8_t* const pMemoryAddress) const;
    error_code validate_offset(ADDRESS_OFFSET memoryOffset) const;
    error_code validate_size(size_t memorySize) const;

    // Gets the offset corresponding to a memory address.
    ADDRESS_OFFSET get_offset(const uint8_t* const pMemoryAddress) const;

    // Gets the memory address corresponding to an offset.
    uint8_t* get_address(ADDRESS_OFFSET memoryOffset) const;

    // Gets the allocation metadata record given the base address of the allocation.
    memory_allocation_metadata* read_allocation_metadata(ADDRESS_OFFSET memoryOffset) const;

    // Get the record corresponding to an offset.
    memory_record* read_memory_record(ADDRESS_OFFSET recordOffset) const;

    // Sets up two references for traversing the given list.
    // If list is empty, pCurrentRecord will be set to nullptr,
    // otherwise it will be set to the list's first node.
    void start(memory_record* pListHead, iteration_context& context) const;

    // Basic iteration method over a MemoryRecord list, using two references.
    // It ensures a safe traversal of the list in a multi-threaded context
    // in which other threads may insert or remove nodes from the list.
    //
    // Returns true if a next node was found and false if we reached the end of the list.
    bool move_next(iteration_context& context) const;

    // In some situations, we need to reset the current node
    // and advance it again from the previous node.
    void reset_current(iteration_context& context) const;

    // Attempts to lock the wanted access on the previous record.
    //
    // If access is locked but the link between the previous record and the next is broken,
    // the lock will be reverted and we'll fail the call.
    //
    // An access mark (a read mark) on a node just ensures that whatever its 'next' link points to
    // is still part of our list, but does not prevent that link from changing.
    // An access lock ensures that the 'next' link cannot be changed by another thread.
    //
    // For Insert/Update, we only need to acquire a lock on pPreviousRecord, to "freeze its next link".
    // For Delete, we need to acquire locks on both pPreviousRecord and pCurrentRecord,
    // because the operation impacts two links.
    //
    // Returns true if access was acquired and false otherwise.
    bool try_to_lock_access(iteration_context& context, access_lock_type wantedAccess, access_lock_type& existingAccess) const;

    bool try_to_lock_access(iteration_context& context, access_lock_type wantedAccess) const;

    // Removes the current record from the list.
    // Assumes that both records have already had their access locked.
    // This method will avoid ABA issues by delaying its completion
    // until all readers have moved past the current record.
    // This and the held locks ensure that the next link of the current node
    // is safe to pursue and still points to a node in our list.
    void remove(iteration_context& context) const;

    // Inserts a record after the previous record specified in the context.
    // Safety of this operation also relies on the calling party
    // locking access to the record after which we insert.
    void insert(iteration_context& context, memory_record*& pRecord) const;

    // Inserts a memory record in order by either offset or size value.
    void insert_memory_record(memory_record* pListHead, memory_record* pMemoryRecord, bool sortByOffset) const;

    protected:

    // The base memory address relative to which we compute our offsets.
    uint8_t* m_base_memory_address;

    // The base memory offset at which our buffer starts (in case we only own a window into a larger memory block).
    ADDRESS_OFFSET m_base_memory_offset;

    // The total size of the memory segment in which we operate.
    size_t m_total_memory_size;

    protected:

    // Helper function that attempts to advance the current record past the previous one.
    //
    // Returns true if it succeeds and false otherwise.
    bool try_to_advance_current_record(iteration_context& context) const;
};

}
}
}

