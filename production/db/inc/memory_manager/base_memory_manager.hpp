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

class base_memory_manager_t
{
    public:

    static const size_t c_memory_alignment = sizeof(uint64_t);

    public:

    base_memory_manager_t();

    // Sanity checks.
    bool validate_address_alignment(const uint8_t* const memory_address) const;
    bool validate_offset_alignment(address_offset_t memory_offset) const;
    bool validate_size_alignment(size_t memory_size) const;

    error_code_t validate_address(const uint8_t* const memory_address) const;
    error_code_t validate_offset(address_offset_t memory_offset) const;
    error_code_t validate_size(size_t memory_size) const;

    // Gets the offset corresponding to a memory address.
    address_offset_t get_offset(const uint8_t* const memory_address) const;

    // Gets the memory address corresponding to an offset.
    uint8_t* get_address(address_offset_t memory_offset) const;

    // Gets the allocation metadata record given the base address of the allocation.
    memory_allocation_metadata_t* read_allocation_metadata(address_offset_t memory_offset) const;

    // Get the record corresponding to an offset.
    memory_record_t* read_memory_record(address_offset_t record_offset) const;

    // Sets up two references for traversing the given list.
    // If list is empty, pCurrentRecord will be set to nullptr,
    // otherwise it will be set to the list's first node.
    void start(memory_record_t* list_head, iteration_context_t& context) const;

    // Basic iteration method over a MemoryRecord list, using two references.
    // It ensures a safe traversal of the list in a multi-threaded context
    // in which other threads may insert or remove nodes from the list.
    //
    // Returns true if a next node was found and false if we reached the end of the list.
    bool move_next(iteration_context_t& context) const;

    // In some situations, we need to reset the current node
    // and advance it again from the previous node.
    void reset_current(iteration_context_t& context) const;

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
    bool try_to_lock_access(iteration_context_t& context, access_lock_type_t wanted_access, access_lock_type_t& existing_access) const;

    bool try_to_lock_access(iteration_context_t& context, access_lock_type_t wanted_access) const;

    // Removes the current record from the list.
    // Assumes that both records have already had their access locked.
    // This method will avoid ABA issues by delaying its completion
    // until all readers have moved past the current record.
    // This and the held locks ensure that the next link of the current node
    // is safe to pursue and still points to a node in our list.
    void remove(iteration_context_t& context) const;

    // Inserts a record after the previous record specified in the context.
    // Safety of this operation also relies on the calling party
    // locking access to the record after which we insert.
    void insert(iteration_context_t& context, memory_record_t*& record) const;

    // Inserts a memory record in order by either offset or size value.
    void insert_memory_record(memory_record_t* list_head, memory_record_t* memory_record, bool sort_by_offset) const;

    protected:

    // The base memory address relative to which we compute our offsets.
    uint8_t* m_base_memory_address;

    // The base memory offset at which our buffer starts (in case we only own a window into a larger memory block).
    address_offset_t m_base_memory_offset;

    // The total size of the memory segment in which we operate.
    size_t m_total_memory_size;

    protected:

    // Helper function that attempts to advance the current record past the previous one.
    //
    // Returns true if it succeeds and false otherwise.
    bool try_to_advance_current_record(iteration_context_t& context) const;
};

}
}
}
