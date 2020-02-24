/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "base_memory_manager.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

class CStackAllocator : public CBaseMemoryManager
{
    friend class CMemoryManager;

    private:

    // Only a CMemoryManager can create CStackAllocator instances.
    CStackAllocator();

    // Sets CStackAllocator execution flags.
    void set_execution_flags(const ExecutionFlags& executionFlags);

    // Initialize the StackAllocator with a specific memory buffer from which to allocate memory.
    // The start of the buffer is specified as an offset from a base address.
    // This is also only callable by a CMemoryManager.
    EMemoryManagerErrorCode initialize(uint8_t* pBaseMemoryAddress, ADDRESS_OFFSET memoryOffset, size_t memorySize);

    public:

    // Allocate a new memory block that will be designated by the provided slot id.
    // The old memory offset of the slot id is also provided, for later garbage collection.
    // A memory size of 0 indicates a deletion, but for clarity, use Delete() method instead.
    EMemoryManagerErrorCode allocate(
        SLOT_ID slotId,
        ADDRESS_OFFSET oldSlotOffset,
        size_t memorySize,
        ADDRESS_OFFSET& allocatedMemoryOffset) const;

    // Memory deallocation method.
    EMemoryManagerErrorCode deallocate(SLOT_ID slotId, ADDRESS_OFFSET slotOffset) const;

    // Deallocate all allocations made after the first N. This allows a transaction to quickly rollback changes.
    EMemoryManagerErrorCode deallocate(size_t countAllocationsToKeep) const;

    // Returns a pointer to the metadata structure, for direct access.
    StackAllocatorMetadata* get_metadata() const;

    // Returns the total number of allocations performed so far (except those already rolled back).
    size_t get_allocation_count() const;

    // Returns a pointer to the N-th allocation record.
    // Allocation numbers start at 1.
    StackAllocatorAllocation* get_allocation_record(size_t allocationNumber) const;

    // Find the offset where we can make the next allocation.
    ADDRESS_OFFSET get_next_allocation_offset() const;

    private:

    // A pointer to our metadata information, stored in the same memory that we manage.
    // Unlike the MemoryManager, which stores its metadata at the start of the buffer,
    // the StackAllocator stores it at the end.
    StackAllocatorMetadata* m_pMetadata;

    // Our execution flags.
    ExecutionFlags m_executionFlags;

    private:

    void output_debugging_information(const string& contextDescription) const;
};

}
}
}

