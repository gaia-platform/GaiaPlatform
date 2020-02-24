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

class CBaseMemoryManager
{
    public:

    static const size_t MEMORY_ALIGNMENT = sizeof(uint64_t);

    public:

    CBaseMemoryManager();

    // Sanity checks.
    bool ValidateAddressAlignment(const uint8_t* const pMemoryAddress) const;
    bool ValidateOffsetAlignment(ADDRESS_OFFSET memoryOffset) const;
    bool ValidateSizeAlignment(size_t memorySize) const;

    EMemoryManagerErrorCode ValidateAddress(const uint8_t* const pMemoryAddress) const;
    EMemoryManagerErrorCode ValidateOffset(ADDRESS_OFFSET memoryOffset) const;
    EMemoryManagerErrorCode ValidateSize(size_t memorySize) const;

    // Gets the offset corresponding to a memory address.
    ADDRESS_OFFSET GetOffset(const uint8_t* const pMemoryAddress) const;

    // Gets the memory address corresponding to an offset.
    uint8_t* GetAddress(ADDRESS_OFFSET memoryOffset) const;

    // Gets the allocation metadata record given the base address of the allocation.
    MemoryAllocationMetadata* ReadAllocationMetadata(ADDRESS_OFFSET memoryOffset) const;

    // Get the record corresponding to an offset.
    MemoryRecord* ReadMemoryRecord(ADDRESS_OFFSET recordOffset) const;

    // Sets up two references for traversing the given list.
    // If list is empty, pCurrentRecord will be set to nullptr,
    // otherwise it will be set to the list's first node.
    void Start(MemoryRecord* pListHead, IterationContext& context) const;

    // Basic iteration method over a MemoryRecord list, using two references.
    // It ensures a safe traversal of the list in a multi-threaded context
    // in which other threads may insert or remove nodes from the list.
    //
    // Returns true if a next node was found and false if we reached the end of the list.
    bool MoveNext(IterationContext& context) const;

    // In some situations, we need to reset the current node
    // and advance it again from the previous node.
    void ResetCurrent(IterationContext& context) const;

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
    bool TryToLockAccess(IterationContext& context, EAccessLockType wantedAccess, EAccessLockType& existingAccess) const;

    bool TryToLockAccess(IterationContext& context, EAccessLockType wantedAccess) const;

    // Removes the current record from the list.
    // Assumes that both records have already had their access locked.
    // This method will avoid ABA issues by delaying its completion
    // until all readers have moved past the current record.
    // This and the held locks ensure that the next link of the current node
    // is safe to pursue and still points to a node in our list.
    void Remove(IterationContext& context) const;

    // Inserts a record after the previous record specified in the context.
    // Safety of this operation also relies on the calling party
    // locking access to the record after which we insert.
    void Insert(IterationContext& context, MemoryRecord*& pRecord) const;

    // Inserts a memory record in order by either offset or size value.
    void InsertMemoryRecord(MemoryRecord* pListHead, MemoryRecord* pMemoryRecord, bool sortByOffset) const;

    protected:

    // The base memory address relative to which we compute our offsets.
    uint8_t* m_pBaseMemoryAddress;

    // The base memory offset at which our buffer starts (in case we only own a window into a larger memory block).
    ADDRESS_OFFSET m_baseMemoryOffset;

    // The total size of the memory segment in which we operate.
    size_t m_totalMemorySize;

    protected:

    // Helper function that attempts to advance the current record past the previous one.
    //
    // Returns true if it succeeds and false otherwise.
    bool TryToAdvanceCurrentRecord(IterationContext& context) const;
};

}
}
}

