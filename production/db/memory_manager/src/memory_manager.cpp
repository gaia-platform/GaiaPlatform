/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <iostream>
#include <sstream>
#include <memory>

#include "constants.hpp"
#include "retail_assert.hpp"
#include "types.hpp"
#include "access_control.hpp"
#include "stack_allocator.hpp"
#include "memory_manager.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db::memory_manager;

CMemoryManager::CMemoryManager() : CBaseMemoryManager()
{
    m_pMetadata = nullptr;

    // Sanity check.
    const size_t expectedMetadataSizeInBytes = 112;
    std::stringstream messageStream;

    messageStream
     << "Metadata information structure representation does not have the expected size on this system: "
     << sizeof(Metadata) << "!";

    retail_assert(
        sizeof(Metadata) == expectedMetadataSizeInBytes,
        messageStream.str());
}

void CMemoryManager::SetExecutionFlags(const ExecutionFlags& executionFlags)
{
    m_executionFlags = executionFlags;
}

EMemoryManagerErrorCode CMemoryManager::Manage(
    uint8_t* pMemoryAddress,
    size_t memorySize,
    size_t mainMemorySystemReservedSize,
    bool initialize)
{
    // Sanity checks.
    if (pMemoryAddress == nullptr || memorySize == 0)
    {
        return mmec_InvalidArgumentValue;
    }

    if (!ValidateAddressAlignment(pMemoryAddress))
    {
        return mmec_MemoryAddressNotAligned;
    }

    if (!ValidateSizeAlignment(memorySize))
    {
        return mmec_MemorySizeNotAligned;
    }

    if (memorySize < sizeof(Metadata) + mainMemorySystemReservedSize
        || sizeof(Metadata) + mainMemorySystemReservedSize < mainMemorySystemReservedSize)
    {
        return mmec_InsufficientMemorySize;
    }

    // Save our parameters.
    m_pBaseMemoryAddress = pMemoryAddress;
    m_totalMemorySize = memorySize;
    m_mainMemorySystemReservedSize = mainMemorySystemReservedSize;

    // Map the metadata information for quick reference.
    m_pMetadata = reinterpret_cast<Metadata*>(m_pBaseMemoryAddress);

    // If necessary, initialize our metadata.
    if (initialize)
    {
        m_pMetadata->Clear(
            sizeof(Metadata),
            m_totalMemorySize);
    }

    if (m_executionFlags.enableConsoleOutput)
    {
        cout << "  Configuration - mainMemorySystemReservedSize = " << m_mainMemorySystemReservedSize << endl;
        cout << "  Configuration - enableExtraValidations = " << m_executionFlags.enableExtraValidations << endl;

        OutputDebuggingInformation("Manage");
    }

    // The master manager is the one that initializes the memory.
    m_isMasterManager = initialize;

    return mmec_Success;
}

EMemoryManagerErrorCode CMemoryManager::Allocate(
    size_t memorySize,
    ADDRESS_OFFSET& allocatedMemoryOffset) const
{
    allocatedMemoryOffset = 0;

    if (m_pMetadata == nullptr)
    {
        return mmec_NotInitialized;
    }

    EMemoryManagerErrorCode errorCode = ValidateSize(memorySize);
    if (errorCode != mmec_Success)
    {
        return errorCode;
    }

    size_t sizeToAllocate = memorySize + sizeof(MemoryAllocationMetadata);

    // First, attempt to reuse freed memory blocks, if possible.
    allocatedMemoryOffset = AllocateFromFreedMemory(sizeToAllocate);

    // Otherwise, fall back to allocating from our main memory block.
    if (allocatedMemoryOffset == 0)
    {
        allocatedMemoryOffset = AllocateFromMainMemory(sizeToAllocate);
    }

    if (m_executionFlags.enableConsoleOutput)
    {
        OutputDebuggingInformation("Allocate");
    }

    if (allocatedMemoryOffset == 0)
    {
        return mmec_InsufficientMemorySize;
    }

    return mmec_Success;
}

EMemoryManagerErrorCode CMemoryManager::CreateStackAllocator(
    size_t memorySize,
    CStackAllocator*& pStackAllocator) const
{
    pStackAllocator = nullptr;

    if (m_pMetadata == nullptr)
    {
        return mmec_NotInitialized;
    }

    ADDRESS_OFFSET memoryOffset = 0;
    EMemoryManagerErrorCode errorCode = Allocate(memorySize, memoryOffset);
    if (errorCode != mmec_Success)
    {
        return errorCode;
    }

    pStackAllocator = new CStackAllocator();

    pStackAllocator->SetExecutionFlags(m_executionFlags);

    errorCode = pStackAllocator->Initialize(m_pBaseMemoryAddress, memoryOffset, memorySize);
    if (errorCode != mmec_Success)
    {
        delete pStackAllocator;
        pStackAllocator = nullptr;
    }

    return errorCode;
}

EMemoryManagerErrorCode CMemoryManager::CommitStackAllocator(
    CStackAllocator* pStackAllocator,
    SERIALIZATION_NUMBER serializationNumber) const
{
    if (m_pMetadata == nullptr)
    {
        return mmec_NotInitialized;
    }

    if (pStackAllocator == nullptr)
    {
        return mmec_InvalidArgumentValue;
    }

    // Ensure that the stack allocator memory gets reclaimed.
    unique_ptr<CStackAllocator> apStackAllocator(pStackAllocator);

    size_t countAllocations = pStackAllocator->GetAllocationCount();

    // Get metadata record for the entire stack allocator memory block.
    MemoryAllocationMetadata* pFirstStackAllocationMetadata = ReadAllocationMetadata(pStackAllocator->m_baseMemoryOffset);
    ADDRESS_OFFSET firstStackAllocationMetadataOffset
        = GetOffset(reinterpret_cast<uint8_t *>(pFirstStackAllocationMetadata));

    // Get metadata for the stack allocator.
    StackAllocatorMetadata* pStackAllocatorMetadata = pStackAllocator->GetMetadata();
    retail_assert(pStackAllocatorMetadata != nullptr, "An unexpected null metadata record was retrieved!");

    // Write serialization number.
    pStackAllocatorMetadata->serializationNumber = serializationNumber;

    if (countAllocations == 0)
    {
        // Special case: all allocations have been reverted, so we need to mark the entire memory block as free.
        if (m_executionFlags.enableExtraValidations)
        {
            retail_assert(
                firstStackAllocationMetadataOffset
                == pStackAllocator->m_baseMemoryOffset - sizeof(MemoryAllocationMetadata),
                "Allocation metadata offset does not match manually computed size!");
            retail_assert(
                pFirstStackAllocationMetadata->allocationSize
                == pStackAllocator->m_totalMemorySize + sizeof(MemoryAllocationMetadata),
                "Allocation metadata size does not match manually computed size!");
        }

        // Try to mark memory as free. This operation can only fail if we run out of memory.
        MemoryRecord* pFreeMemoryRecord
            = GetFreeMemoryRecord(firstStackAllocationMetadataOffset, pFirstStackAllocationMetadata->allocationSize);
        if (pFreeMemoryRecord == nullptr)
        {
            return mmec_InsufficientMemorySize;
        }
        else
        {
            InsertFreeMemoryRecord(pFreeMemoryRecord);
        }
    }
    else
    {
        // Iterate over all StackAllocator allocations and collect old memory offsets in free memory records.
        // However, we will not insert any of these records into the free memory list
        // until we know that our processing can no longer fail.
        unique_ptr<MemoryRecord*[]> argFreeMemoryRecords(new MemoryRecord*[countAllocations]());
        for (size_t allocationNumber = 1; allocationNumber <= countAllocations; allocationNumber++)
        {
            StackAllocatorAllocation* pAllocationRecord = pStackAllocator->GetAllocationRecord(allocationNumber);
            retail_assert(pAllocationRecord != nullptr, "An unexpected null allocation record was retrieved!");

            if (pAllocationRecord->oldMemoryOffset != 0)
            {
                MemoryAllocationMetadata* pAllocationMetadata = ReadAllocationMetadata(pAllocationRecord->oldMemoryOffset);
                ADDRESS_OFFSET allocationMetadataOffset = GetOffset(reinterpret_cast<uint8_t*>(pAllocationMetadata));

                // Mark memory block as free.
                // If we cannot do this, then we ran out of memory;
                // in that case we'll just reclaim all records we collected so far.
                MemoryRecord* pFreeMemoryRecord
                    = GetFreeMemoryRecord(allocationMetadataOffset, pAllocationMetadata->allocationSize);
                if (pFreeMemoryRecord == nullptr)
                {
                    ReclaimRecords(argFreeMemoryRecords.get(), countAllocations);
                    return mmec_InsufficientMemorySize;
                }
                else
                {
                    argFreeMemoryRecords[allocationNumber - 1] = pFreeMemoryRecord;
                }
                
            }
        }

        // Insert metadata block in the unserialized allocation list.
        // If we fail, reclaim all the records that we have collected so far.
        // But if we succeed, then we can insert all our collected records into the list of free memory records.
        EMemoryManagerErrorCode errorCode = TrackStackAllocatorMetadataForSerialization(pStackAllocatorMetadata);
        if (errorCode != mmec_Success)
        {
            ReclaimRecords(argFreeMemoryRecords.get(), countAllocations);
            return errorCode;
        }
        else
        {
            InsertFreeMemoryRecords(argFreeMemoryRecords.get(), countAllocations);
        }

        // Patch metadata information of the first allocation.
        // We will deallocate the unused memory after serialization,
        // because the stack allocator metadata is still needed until then
        // and is now tracked by the unserialized allocation list.
        pFirstStackAllocationMetadata->allocationSize
            = pStackAllocatorMetadata->firstAllocationSize + sizeof(MemoryAllocationMetadata);
    }

    if (m_executionFlags.enableConsoleOutput)
    {
        OutputDebuggingInformation("CommitStackAllocator");
        pStackAllocator->OutputDebuggingInformation("CommitStackAllocator");
    }

    return mmec_Success;
}

EMemoryManagerErrorCode CMemoryManager::GetUnserializedAllocationsListHead(MemoryListNode*& pListHead) const
{
    if (m_pMetadata == nullptr)
    {
        return mmec_NotInitialized;
    }

    // Serializing allocations should only be performed by the database engine
    // through its master manager instance.
    if (!m_isMasterManager)
    {
        return mmec_OperationAvailableOnlyToMasterManager;
    }

    if (m_executionFlags.enableConsoleOutput)
    {
        OutputDebuggingInformation("GetUnserializedAllocationsListHead");
    }

    pListHead = &m_pMetadata->unserializedAllocationsListHead;

    return mmec_Success;
}

EMemoryManagerErrorCode CMemoryManager::UpdateUnserializedAllocationsListHead(
        ADDRESS_OFFSET nextUnserializedAllocationRecordOffset) const
{
    if (m_pMetadata == nullptr)
    {
        return mmec_NotInitialized;
    }

    // We require an offset because the end of the list may not be safe to process,
    // due to other threads making insertions.
    if (nextUnserializedAllocationRecordOffset == 0)
    {
        return mmec_InvalidArgumentValue;
    }

    // Serializing allocations should only be performed by the database engine
    // through its master manager instance.
    if (!m_isMasterManager)
    {
        return mmec_OperationAvailableOnlyToMasterManager;
    }

    ADDRESS_OFFSET currentRecordOffset = m_pMetadata->unserializedAllocationsListHead.next;
    retail_assert(currentRecordOffset != 0, "UpdateUnserializedAllocationsListHead() was called on an empty list!");
    retail_assert(
        currentRecordOffset != nextUnserializedAllocationRecordOffset,
        "UpdateUnserializedAllocationsListHead() was called on an already updated list!");

    while (currentRecordOffset != nextUnserializedAllocationRecordOffset)
    {
        // Get the actual record.
        MemoryRecord* pCurrentRecord = CBaseMemoryManager::ReadMemoryRecord(currentRecordOffset);

        // Get the StackAllocator metadata.
        ADDRESS_OFFSET currentMetadataOffset = pCurrentRecord->memoryOffset;
        uint8_t* pCurrentMetadataAddress = GetAddress(currentMetadataOffset);
        StackAllocatorMetadata* pCurrentMetadata = reinterpret_cast<StackAllocatorMetadata*>(pCurrentMetadataAddress);

        // Determine the boundaries of the memory block that we can free from the StackAllocator.
        ADDRESS_OFFSET startMemoryOffset = pCurrentMetadata->nextAllocationOffset;
        ADDRESS_OFFSET endMemoryOffset = currentMetadataOffset + sizeof(StackAllocatorMetadata);
        retail_assert(ValidateOffset(startMemoryOffset) == mmec_Success, "Calculated start memory offset is invalid");
        retail_assert(ValidateOffset(endMemoryOffset) == mmec_Success, "Calculated end memory offset is invalid");

        size_t memorySize = endMemoryOffset - startMemoryOffset;

        // Advance current record offset to next record, to prepare for the next iteration.
        currentRecordOffset = pCurrentRecord->next;

        // Also advance the list head to the next record.
        // This means that when the loop condition is reached,
        // the list head will already be updated to reference the next unserialized record.
        m_pMetadata->unserializedAllocationsListHead.next = currentRecordOffset;

        // We'll repurpose the current record as a free memory record,
        // so we'll update it to track the free memory block information.
        pCurrentRecord->memoryOffset = startMemoryOffset;
        pCurrentRecord->memorySize = memorySize;
        pCurrentRecord->next = 0;

        // Insert the record in the free memory list.
        InsertFreeMemoryRecord(pCurrentRecord);
    }

    retail_assert(
        m_pMetadata->unserializedAllocationsListHead.next == nextUnserializedAllocationRecordOffset,
        "UpdateUnserializedAllocationsListHead() has not properly updated the list!");

    if (m_executionFlags.enableConsoleOutput)
    {
        OutputDebuggingInformation("UpdateUnserializedAllocationsListHead");
    }

    return mmec_Success;
}

size_t CMemoryManager::GetMainMemoryAvailableSize(bool includeSystemReservedSize) const
{
    size_t availableSize = 0;

    // Ignore return value; we just want the available size.
    IsMainMemoryExhausted(
        m_pMetadata->startMainAvailableMemory,
        m_pMetadata->lowestMetadataMemoryUse,
        includeSystemReservedSize,
        availableSize);

    return availableSize;
}

bool CMemoryManager::IsMainMemoryExhausted(
    ADDRESS_OFFSET startMemoryOffset,
    ADDRESS_OFFSET endMemoryOffset,
    bool includeSystemReservedSize) const
{
    size_t availableSize = 0;

    return IsMainMemoryExhausted(
        startMemoryOffset,
        endMemoryOffset,
        includeSystemReservedSize,
        availableSize);
}

bool CMemoryManager::IsMainMemoryExhausted(
    ADDRESS_OFFSET startMemoryOffset,
    ADDRESS_OFFSET endMemoryOffset,
    bool includeSystemReservedSize,
    size_t& availableSize) const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");

    availableSize = 0;

    size_t reservedSize = includeSystemReservedSize ? 0 : m_mainMemorySystemReservedSize;

    if (startMemoryOffset + reservedSize > endMemoryOffset
        || startMemoryOffset + reservedSize < startMemoryOffset)
    {
        return true;
    }

    availableSize = endMemoryOffset - startMemoryOffset - reservedSize;

    return false;
}

ADDRESS_OFFSET CMemoryManager::ProcessAllocation(ADDRESS_OFFSET allocationOffset, size_t sizeToAllocate) const
{
    retail_assert(allocationOffset != 0, "ProcessAllocation() was called for an empty allocation!");

    // Write the allocation metadata.
    uint8_t* pAllocationMetadataAddress = GetAddress(allocationOffset);
    MemoryAllocationMetadata* pAllocationMetadata
        = reinterpret_cast<MemoryAllocationMetadata*>(pAllocationMetadataAddress);
    pAllocationMetadata->allocationSize = sizeToAllocate;

    // We return the offset past the metadata.
    allocationOffset += sizeof(MemoryAllocationMetadata);
    return allocationOffset;
}

ADDRESS_OFFSET CMemoryManager::AllocateFromMainMemory(size_t sizeToAllocate) const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");

    // Main memory allocations should not use the reserved space.
    bool includeSystemReservedSize = false;

    // If the allocation exhausts our memory, we cannot perform it.
    if (GetMainMemoryAvailableSize(includeSystemReservedSize) < sizeToAllocate)
    {
        return 0;
    }

    // Claim the space.
    ADDRESS_OFFSET oldStartMainAvailableMemory = __sync_fetch_and_add(
        &m_pMetadata->startMainAvailableMemory,
        sizeToAllocate);
    ADDRESS_OFFSET newStartMainAvailableMemory = oldStartMainAvailableMemory + sizeToAllocate;

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (IsMainMemoryExhausted(
        newStartMainAvailableMemory,
        m_pMetadata->lowestMetadataMemoryUse,
        includeSystemReservedSize))
    {
        // We exhausted the main memory so we must undo our update.
        while (!__sync_bool_compare_and_swap(
            &m_pMetadata->startMainAvailableMemory,
            newStartMainAvailableMemory,
            oldStartMainAvailableMemory))
        {
            // A failure indicates that another thread has done the same.
            // Sleep to allow it to undo its change, so we can do the same.
            // This is safe because metadata allocations are never reverted
            // so all other subsequent memory allocations will also attempt to revert back.
            usleep(1);
        }

        return 0;
    }

    // Our allocation has succeeded.
    ADDRESS_OFFSET allocationOffset = oldStartMainAvailableMemory;
    ADDRESS_OFFSET adjustedAllocationOffset = ProcessAllocation(allocationOffset, sizeToAllocate);

    if (m_executionFlags.enableConsoleOutput)
    {
        cout << endl << "Allocated " << sizeToAllocate << " bytes at offset " << allocationOffset;
        cout << " from main memory." << endl;
    }

    return adjustedAllocationOffset;
}

ADDRESS_OFFSET CMemoryManager::AllocateFromFreedMemory(size_t sizeToAllocate) const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");

    IterationContext context;
    Start(&m_pMetadata->freeMemoryListHead, context);
    ADDRESS_OFFSET allocationOffset = 0;

    // Iterate over the free memory list and try to find a large enough block for this allocation.
    while (context.pCurrentRecord != nullptr)
    {
        if (context.pCurrentRecord->memorySize >= sizeToAllocate)
        {
            // We have 2 situations: the memory block is exactly our required size or it is larger.
            // In both cases, we need to check the size of the block again after acquiring the locks,
            // because another thread may have managed to update it before we could lock it. 
            if (context.pCurrentRecord->memorySize == sizeToAllocate)
            {
                if (TryToLockAccess(context, alt_UpdateRemove)
                    && context.autoAccessCurrentRecord.TryToLockAccess(alt_Remove)
                    && context.pCurrentRecord->memorySize == sizeToAllocate)
                {

                    retail_assert(
                        context.pCurrentRecord->memorySize == sizeToAllocate,
                        "Internal error - we are attempting to allocate from a block that is too small!");

                    // Jolly good! We've found a memory block of exactly the size that we were looking for.
                    Remove(context);

                    allocationOffset = context.pCurrentRecord->memoryOffset;

                    // Reclaim the memory record for later reuse;
                    InsertReclaimedMemoryRecord(context.pCurrentRecord);

                    break;
                }
            }
            else
            {
                if (TryToLockAccess(context, alt_Update)
                    && context.pCurrentRecord->memorySize > sizeToAllocate)
                {
                    retail_assert(
                        context.pCurrentRecord->memorySize > sizeToAllocate,
                        "Internal error - we are attempting to allocate from a block that is too small!");

                    allocationOffset = context.pCurrentRecord->memoryOffset;

                    // We have more space than we needed, so update the record
                    // to track the remaining space.
                    context.pCurrentRecord->memoryOffset += sizeToAllocate;
                    context.pCurrentRecord->memorySize -= sizeToAllocate;

                    break;
                }
            }
        }

        MoveNext(context);
    }

    if (allocationOffset == 0)
    {
        return 0;
    }

    ADDRESS_OFFSET adjustedAllocationOffset = ProcessAllocation(allocationOffset, sizeToAllocate);

    if (m_executionFlags.enableConsoleOutput)
    {
        cout << endl << "Allocated " << sizeToAllocate << " bytes at offset " << allocationOffset;
        cout << " from freed memory." << endl;
    }

    return adjustedAllocationOffset;
}

MemoryRecord* CMemoryManager::GetMemoryRecord() const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");

    MemoryRecord* pFreeMemoryRecord = GetReclaimedMemoryRecord();

    if (pFreeMemoryRecord == nullptr)
    {
        pFreeMemoryRecord = GetNewMemoryRecord();
    }

    // The record should not indicate any access.
    retail_assert(
        pFreeMemoryRecord == nullptr
        || pFreeMemoryRecord->accessControl.readersCount == 0,
        "The readers count of a new memory record should be 0!");
    retail_assert(
        pFreeMemoryRecord == nullptr
        || pFreeMemoryRecord->accessControl.accessLock == alt_None,
        "The access lock of a new memory record should be None!");

    return pFreeMemoryRecord;
}

MemoryRecord* CMemoryManager::GetNewMemoryRecord() const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");

    // Metadata allocations can use the reserved space.
    bool includeSystemReservedSize = true;

    // If the allocation exhausts our memory, we cannot perform it.
    if (GetMainMemoryAvailableSize(includeSystemReservedSize) < sizeof(MemoryRecord))
    {
        return nullptr;
    }

    // Claim the space.
    ADDRESS_OFFSET oldLowestMetadataMemoryUse = __sync_fetch_and_sub(
        &m_pMetadata->lowestMetadataMemoryUse,
        sizeof(MemoryRecord));
    ADDRESS_OFFSET newLowestMetadataMemoryUse = oldLowestMetadataMemoryUse - sizeof(MemoryRecord);

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (IsMainMemoryExhausted(
        m_pMetadata->startMainAvailableMemory,
        newLowestMetadataMemoryUse,
        includeSystemReservedSize))
    {
        // We exhausted the main memory.
        // Attempting to revert the update to m_pMetadata->lowestMetadataMemoryUse
        // can conflict with attempts to revert updates to m_pMetadata->startMainAvailableMemory,
        // so just leave it as is (yes, we'll potentially lose a record's chunk of memory!)
        // and fail the record allocation.
        return nullptr;
    }

    // Our allocation has succeeded.
    ADDRESS_OFFSET recordOffset = newLowestMetadataMemoryUse;

    if (m_executionFlags.enableConsoleOutput)
    {
        cout << endl << "Allocated offset " << recordOffset << " for a new memory record." << endl;
    }

    MemoryRecord* pFreeMemoryRecord = CBaseMemoryManager::ReadMemoryRecord(recordOffset);

    // This is uninitialized memory, so we need to explicitly clear it.
    pFreeMemoryRecord->Clear();

    return pFreeMemoryRecord;
}

MemoryRecord* CMemoryManager::GetReclaimedMemoryRecord() const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");

    IterationContext context;
    Start(&m_pMetadata->reclaimedRecordsListHead, context);
    MemoryRecord* pReclaimedRecord = nullptr;

    // Iterate through the list of reclaimed records and attempt to extract one.
    while (context.pCurrentRecord != nullptr)
    {
        // Retrieving a node from this list is going to be a small-effort initiative.
        // If we fail, we'll just allocate a new node, so eventually,
        // enough nodes will get inserted into this list that we'll succeed easily to remove one.
        //
        // We'll try to lock each node one after the other.
        if (TryToLockAccess(context, alt_UpdateRemove)
            && context.autoAccessCurrentRecord.TryToLockAccess(alt_Remove))
        {
            Remove(context);

            pReclaimedRecord = context.pCurrentRecord;

            break;
        }

        MoveNext(context);
    }

    if (pReclaimedRecord != nullptr
        && m_executionFlags.enableConsoleOutput)
    {
        uint8_t* pReclaimedRecordAddress = reinterpret_cast<uint8_t*>(pReclaimedRecord);
        ADDRESS_OFFSET reclaimedRecordOffset = GetOffset(pReclaimedRecordAddress);

        cout << endl << "Reclaimed memory record from offset " << reclaimedRecordOffset << "." << endl;
    }

    return pReclaimedRecord;
}

void CMemoryManager::InsertFreeMemoryRecord(MemoryRecord* pFreeMemoryRecord) const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");
    retail_assert(pFreeMemoryRecord != nullptr, "InsertFreeMemoryRecord() was called with a null parameter!");

    bool sortByOffset = true;
    InsertMemoryRecord(&m_pMetadata->freeMemoryListHead, pFreeMemoryRecord, sortByOffset);
}

void CMemoryManager::InsertReclaimedMemoryRecord(MemoryRecord* pReclaimedMemoryRecord) const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");
    retail_assert(pReclaimedMemoryRecord != nullptr, "InsertReclaimedMemoryRecord() was called with a null parameter!");

    IterationContext context;
    Start(&m_pMetadata->reclaimedRecordsListHead, context);

    // We'll keep trying to insert at the beginning of the list.
    while (true)
    {
        if (TryToLockAccess(context, alt_Insert))
        {
            Insert(context, pReclaimedMemoryRecord);

            return;
        }
    }
}

void CMemoryManager::InsertUnserializedAllocationsRecord(MemoryRecord* pUnserializedAllocationsRecord) const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");
    retail_assert(pUnserializedAllocationsRecord != nullptr, "InsertUnserializedAllocationsRecord() was called with a null parameter!");

    // In this case we want to sort by the size value, which holds the serialization number.
    bool sortByOffset = false;
    InsertMemoryRecord(&m_pMetadata->unserializedAllocationsListHead, pUnserializedAllocationsRecord, sortByOffset);
}

MemoryRecord* CMemoryManager::GetFreeMemoryRecord(ADDRESS_OFFSET memoryOffset, size_t memorySize) const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");

    MemoryRecord* pFreeMemoryRecord = GetMemoryRecord();

    if (pFreeMemoryRecord == nullptr)
    {
        return nullptr;
    }

    pFreeMemoryRecord->memoryOffset = memoryOffset;
    pFreeMemoryRecord->memorySize = memorySize;

    return pFreeMemoryRecord;
}

void CMemoryManager::ProcessFreeMemoryRecords(MemoryRecord** freeMemoryRecords, size_t size, bool markAsFree) const
{
    retail_assert(freeMemoryRecords != nullptr, "InsertFreeMemoryRecords() has been called with a null parameter!");

    for (size_t i = 0; i < size; i++)
    {
        if (freeMemoryRecords[i] != nullptr)
        {
            if (markAsFree)
            {
                InsertFreeMemoryRecord(freeMemoryRecords[i]);
            }
            else
            {
                InsertReclaimedMemoryRecord(freeMemoryRecords[i]);
            }
        }
    }
}

void CMemoryManager::InsertFreeMemoryRecords(MemoryRecord** freeMemoryRecords, size_t size) const
{
    return ProcessFreeMemoryRecords(freeMemoryRecords, size, true);
}

void CMemoryManager::ReclaimRecords(MemoryRecord** freeMemoryRecords, size_t size) const
{
    return ProcessFreeMemoryRecords(freeMemoryRecords, size, false);
}

EMemoryManagerErrorCode CMemoryManager::TrackStackAllocatorMetadataForSerialization(
    StackAllocatorMetadata* pStackAllocatorMetadata) const
{
    retail_assert(m_pMetadata != nullptr, "Memory manager has not been initialized!");

    MemoryRecord* pMemoryRecord = GetMemoryRecord();

    if (pMemoryRecord == nullptr)
    {
        return mmec_InsufficientMemorySize;
    }

    uint8_t* pMetadataAddress = reinterpret_cast<uint8_t*>(pStackAllocatorMetadata);
    ADDRESS_OFFSET metadataOffset = GetOffset(pMetadataAddress);

    pMemoryRecord->memoryOffset = metadataOffset;

    // We don't need this field for its original purpose,
    // so we'll repurpose it to hold the serialization number,
    // to make it easier to check this value during list insertions.
    pMemoryRecord->memorySize = pStackAllocatorMetadata->serializationNumber;

    InsertUnserializedAllocationsRecord(pMemoryRecord);

    return mmec_Success;
}

void CMemoryManager::OutputDebuggingInformation(const string& contextDescription) const
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "Debugging output for context: " << contextDescription << ":" << endl;

    if (m_pMetadata == nullptr)
    {
        cout << "  Memory manager has not been initialized." << endl;
    }

    cout << "  Main memory start = " << m_pMetadata->startMainAvailableMemory << endl;
    cout << "  Metadata start = " << m_pMetadata->lowestMetadataMemoryUse << endl;
    cout << "  Available main memory including reserved space = " << GetMainMemoryAvailableSize(true) << endl;
    cout << "  Available main memory excluding reserved space = " << GetMainMemoryAvailableSize(false) << endl;
    cout << "  Free memory list: " << endl;
    OutputListContent(m_pMetadata->freeMemoryListHead);
    cout << "  Reclaimed records list: " << endl;
    OutputListContent(m_pMetadata->reclaimedRecordsListHead); 
    cout << "  Unserialized allocations list: " << endl;
    OutputListContent(m_pMetadata->unserializedAllocationsListHead);
    cout << c_debug_output_separator_line_end << endl;
}

void CMemoryManager::OutputListContent(MemoryRecord listHead) const
{
    size_t recordCount = 0;
    ADDRESS_OFFSET currentRecordOffset = listHead.next;
    while (currentRecordOffset != 0)
    {
        recordCount++;

        MemoryRecord* pCurrentRecord = ReadMemoryRecord(currentRecordOffset);

        cout << "    Record[" << recordCount << "] at offset " << currentRecordOffset << ":" << endl;
        cout << "      offset = " << pCurrentRecord->memoryOffset;
        cout << " size = " << pCurrentRecord->memorySize;
        cout << " readersCount = " << pCurrentRecord->accessControl.readersCount;
        cout << " accessLock = " << pCurrentRecord->accessControl.accessLock;
        cout << " next = " << pCurrentRecord->next << endl;

        currentRecordOffset = pCurrentRecord->next;
    }

    cout << "    List contains " << recordCount << " records." << endl;
}
