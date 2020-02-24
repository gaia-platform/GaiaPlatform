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

class CMemoryManager : public CBaseMemoryManager
{
    public:

    CMemoryManager();

    // Sets CMemoryManager execution flags.
    void SetExecutionFlags(const ExecutionFlags& executionFlags);

    // Tells the memory manager which memory area it should manage.
    //
    // mainMemorySystemReservedSize specifies a minimum size below which the available memory will not be allowed to drop.
    // Once this limit is reached, it can only be used by additional metadata allocations.
    // If exhausted, we're out of memory.
    //
    // If 'initialize' is set to true, the memory is considered to be uninitialized;
    // if not, then the manager will deal with it as if it was already initialized by another memory manager
    // - this means that parts of it are already allocated and in use.
    //
    // All addresses will be offsets relative to the beginning of this block, represented as ADDRESS_OFFSET.
    EMemoryManagerErrorCode Manage(
        uint8_t* pMemoryAddress,
        size_t memorySize,
        size_t mainMemorySystemReservedSize,
        bool initialize = true);

    // Allocates a new block of memory.
    // This interface is meant to be used when the size of the block is known in advance,
    // such as in the case when we load the database objects from disk.
    EMemoryManagerErrorCode Allocate(size_t memorySize, ADDRESS_OFFSET& allocatedMemoryOffset) const;

    // Creates a StackAllocator object that will be used with transactions.
    // This interface will be used to allocate memory during regular database operation.
    EMemoryManagerErrorCode CreateStackAllocator(size_t memorySize, CStackAllocator*& pStackAllocator) const;

    // Once a transaction commits, calling this method will achieve several goals:
    // (i) it will insert the block into a linked list of allocations
    // that is sorted by the associated serialization number.
    // (ii) it will add all old memory locations to the list of free memory.
    EMemoryManagerErrorCode CommitStackAllocator(
        CStackAllocator* pStackAllocator,
        SERIALIZATION_NUMBER serializationNumber) const;

    // This method should be called by the database engine serialization code.
    // This method exposes the list of allocations that is yet to be serialized to disk.
    EMemoryManagerErrorCode GetUnserializedAllocationsListHead(MemoryListNode*& pListHead) const;

    // This method should be called by the database engine serialization code.
    // This method updates the next unserialized allocation at the end of a serialization.
    // We can now pack all serialized allocations before updating our list head.
    //
    // This method is not thread-safe! It assumes it is called from a single thread.
    // The method is thread-safe only with respect to the other memory manager operations.
    EMemoryManagerErrorCode UpdateUnserializedAllocationsListHead(
        ADDRESS_OFFSET nextUnserializedAllocationRecordOffset) const;

    private:

    // All data tracked by the memory manager can be accessed from here.
    // The metadata information structure is stored at the beginning of the managed memory, 
    // but its lists are allocated at the end of it, so they will grow towards the beginning.
    // Also, because the metadata structure is located at offset 0, that offset can be considered to be invalid.
    struct Metadata
    {
        ADDRESS_OFFSET startMainAvailableMemory;
        ADDRESS_OFFSET lowestMetadataMemoryUse;

        MemoryRecord freeMemoryListHead;
        MemoryRecord reclaimedRecordsListHead;
        MemoryRecord unserializedAllocationsListHead;

        void Clear(ADDRESS_OFFSET low, ADDRESS_OFFSET high)
        {
            startMainAvailableMemory = low;
            lowestMetadataMemoryUse = high;

            freeMemoryListHead.Clear();
            reclaimedRecordsListHead.Clear();
            unserializedAllocationsListHead.Clear();
        }
    };

    private:

    // A pointer to our metadata information, stored in the same memory that we manage.
    Metadata* m_pMetadata;

    // There should be a single master manager called by the database engine.
    // Some functionality is restricted to this instance.
    bool m_isMasterManager;

    // See Manage() comment.
    size_t m_mainMemorySystemReservedSize;

    // Our execution flags.
    ExecutionFlags m_executionFlags;

    private:

    size_t GetMainMemoryAvailableSize(bool includeSystemReservedSize) const;

    bool IsMainMemoryExhausted(
        ADDRESS_OFFSET startMemoryOffset,
        ADDRESS_OFFSET endMemoryOffset,
        bool includeSystemReservedSize) const;

    bool IsMainMemoryExhausted(
        ADDRESS_OFFSET startMemoryOffset,
        ADDRESS_OFFSET endMemoryOffset,
        bool includeSystemReservedSize,
        size_t& availableSize) const;

    // Given an allocation offset, set up the allocation metadata and returns the offset past it.
    ADDRESS_OFFSET ProcessAllocation(ADDRESS_OFFSET allocationOffset, size_t sizeToAllocate) const;

    // Attempt to allocate from our main memory block.
    ADDRESS_OFFSET AllocateFromMainMemory(size_t sizeToAllocate) const;

    // Attempt to allocate from one of the already allocated and freed memory blocks.
    ADDRESS_OFFSET AllocateFromFreedMemory(size_t sizeToAllocate) const;

    // Get a memory record.
    MemoryRecord* GetMemoryRecord() const;

    // Attempt to allocate a new record from our main memory block.
    MemoryRecord* GetNewMemoryRecord() const;

    // Attempt to allocate from one of the previously allocated memory records.
    MemoryRecord* GetReclaimedMemoryRecord() const;

    // Insert free memory record in its proper place in the list of free memory records.
    void InsertFreeMemoryRecord(MemoryRecord* pFreeMemoryRecord) const;

    // Insert memory record in the list of reclaimed memory records.
    void InsertReclaimedMemoryRecord(MemoryRecord* pReclaimedMemoryRecord) const;

    // Insert unserialized allocations record in the list of unserialized allocations.
    void InsertUnserializedAllocationsRecord(MemoryRecord* pUnserializedAllocationsRecord) const;

    // Mark memory as free by setting up a free memory record to track it.
    MemoryRecord* GetFreeMemoryRecord(ADDRESS_OFFSET memoryOffset, size_t memorySize) const;

    // Process a list of records by either adding its elements to the list of free memory records
    // or by reclaiming them.
    // This is only meant to be used by the following two methods - call them instead of this.
    void ProcessFreeMemoryRecords(MemoryRecord** freeMemoryRecords, size_t size, bool markAsFree) const;

    // Insert records in the list of free memory records.
    void InsertFreeMemoryRecords(MemoryRecord** freeMemoryRecords, size_t size) const;

    // Insert records in the list of reclaimed memory records.
    void ReclaimRecords(MemoryRecord** freeMemoryRecords, size_t size) const;

    // Track the StackAllocator metadata for future serialization.
    EMemoryManagerErrorCode TrackStackAllocatorMetadataForSerialization(
        StackAllocatorMetadata* pStackAllocatorMetadata) const;

    void OutputDebuggingInformation(const string& contextDescription) const;

    void OutputListContent(MemoryRecord listHead) const;
};

}
}
}

