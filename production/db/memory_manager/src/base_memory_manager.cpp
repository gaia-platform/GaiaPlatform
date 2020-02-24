/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include "retail_assert.hpp"
#include "base_memory_manager.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;

CBaseMemoryManager::CBaseMemoryManager()
{
    m_pBaseMemoryAddress = nullptr;
    m_baseMemoryOffset = 0;
    m_totalMemorySize = 0;
}

bool CBaseMemoryManager::ValidateAddressAlignment(const uint8_t* const pMemoryAddress) const
{
    size_t memoryAddressAsInteger = reinterpret_cast<size_t>(pMemoryAddress);
    return (memoryAddressAsInteger % MEMORY_ALIGNMENT == 0);
}

bool CBaseMemoryManager::ValidateOffsetAlignment(ADDRESS_OFFSET memoryOffset) const
{
    return (memoryOffset % MEMORY_ALIGNMENT == 0);
}

bool CBaseMemoryManager::ValidateSizeAlignment(size_t memorySize) const
{
    return (memorySize % MEMORY_ALIGNMENT == 0);
}

EMemoryManagerErrorCode CBaseMemoryManager::ValidateAddress(const uint8_t* const pMemoryAddress) const
{
    if (!ValidateAddressAlignment(pMemoryAddress))
    {
        return mmec_MemoryAddressNotAligned;
    }

    if (pMemoryAddress < m_pBaseMemoryAddress + m_baseMemoryOffset
        || pMemoryAddress > m_pBaseMemoryAddress + m_baseMemoryOffset + m_totalMemorySize)
    {
        return mmec_MemoryAddressOutOfRange;
    }

    return mmec_Success;
}

EMemoryManagerErrorCode CBaseMemoryManager::ValidateOffset(ADDRESS_OFFSET memoryOffset) const
{
    if (!ValidateOffsetAlignment(memoryOffset))
    {
        return mmec_MemoryOffsetNotAligned;
    }

    if (memoryOffset < m_baseMemoryOffset
        || memoryOffset > m_baseMemoryOffset + m_totalMemorySize)
    {
        return mmec_MemoryOffsetOutOfRange;
    }

    return mmec_Success;
}

EMemoryManagerErrorCode CBaseMemoryManager::ValidateSize(size_t memorySize) const
{
    if (!ValidateSizeAlignment(memorySize))
    {
        return mmec_MemorySizeNotAligned;
    }

    if (memorySize == 0)
    {
        return mmec_MemorySizeCannotBeZero;
    }

    if (memorySize > m_totalMemorySize)
    {
        return mmec_MemorySizeTooLarge;
    }

    return mmec_Success;
}

ADDRESS_OFFSET CBaseMemoryManager::GetOffset(const uint8_t* const pMemoryAddress) const
{
    retail_assert(
        ValidateAddress(pMemoryAddress) == mmec_Success,
        "GetOffset() was called with an invalid address!");

    size_t offset = pMemoryAddress - m_pBaseMemoryAddress;

    return offset;
}

uint8_t* CBaseMemoryManager::GetAddress(ADDRESS_OFFSET memoryOffset) const
{
    retail_assert(
        ValidateOffset(memoryOffset) == mmec_Success,
        "GetAddress() was called with an invalid offset!");

    uint8_t* pMemoryAddress = m_pBaseMemoryAddress + memoryOffset;

    return pMemoryAddress;
}

MemoryAllocationMetadata* CBaseMemoryManager::ReadAllocationMetadata(ADDRESS_OFFSET memoryOffset) const
{
    retail_assert(
        ValidateOffset(memoryOffset) == mmec_Success,
        "GetAllocationMetadata() was called with an invalid offset!");

    retail_assert(
        memoryOffset >= sizeof(MemoryAllocationMetadata),
        "GetAllocationMetadata() was called with an offset that is too small!");

    ADDRESS_OFFSET allocationMetadataOffset = memoryOffset - sizeof(MemoryAllocationMetadata);
    uint8_t* pAllocationMetadataAddress = GetAddress(allocationMetadataOffset);
    MemoryAllocationMetadata* pAllocationMetadata
        = reinterpret_cast<MemoryAllocationMetadata*>(pAllocationMetadataAddress);

    return pAllocationMetadata;
}

MemoryRecord* CBaseMemoryManager::ReadMemoryRecord(ADDRESS_OFFSET recordOffset) const
{
    retail_assert(
        ValidateOffset(recordOffset) == mmec_Success,
        "GetFreeMemoryRecord was called with an invalid offset!");

    retail_assert(
        recordOffset > 0,
        "GetFreeMemoryRecord was called with a zero offset!");

    uint8_t* pRecordAddress = GetAddress(recordOffset);
    MemoryRecord* pRecord = reinterpret_cast<MemoryRecord*>(pRecordAddress);

    return pRecord;
}

bool CBaseMemoryManager::TryToAdvanceCurrentRecord(IterationContext& context) const
{
    retail_assert(context.pPreviousRecord != nullptr, "Previous record cannot be null!");

    ADDRESS_OFFSET currentRecordOffset = context.pPreviousRecord->next;

    if (currentRecordOffset == 0)
    {
        context.pCurrentRecord = nullptr;
        context.autoAccessCurrentRecord.ReleaseAccess();

        return true;
    }

    context.pCurrentRecord = ReadMemoryRecord(currentRecordOffset);
    context.autoAccessCurrentRecord.MarkAccess(&context.pCurrentRecord->accessControl);

    // Now that we marked our access, we need to check
    // whether pCurrentRecord still belongs in our list,
    // because another thread may have removed it
    // before we got to mark our access.
    if (context.pPreviousRecord->next == currentRecordOffset)
    {
        // All looks good, so we're done here.
        return true;
    }

    // We failed, so we should release the access that we marked.
    context.autoAccessCurrentRecord.ReleaseAccess();

    return false;
}

void CBaseMemoryManager::Start(MemoryRecord* pListHead, IterationContext& context) const
{
    retail_assert(pListHead != nullptr, "Null list head was passed to CBaseMemoryManager::Start()!");

    context.pPreviousRecord = pListHead;
    context.autoAccessPreviousRecord.MarkAccess(&context.pPreviousRecord->accessControl);

    // Keep trying to advance current record until we succeed.
    while (!TryToAdvanceCurrentRecord(context))
    {
    }
}

bool CBaseMemoryManager::MoveNext(IterationContext& context) const
{
    retail_assert(context.pPreviousRecord != nullptr, "Previous record cannot be null!");
    retail_assert(context.autoAccessPreviousRecord.HasMarkedAccess(), "Access to previous record has not been marked yet!");

    if (context.pCurrentRecord == nullptr)
    {
        return false;
    }

    retail_assert(context.autoAccessCurrentRecord.HasMarkedAccess(), "Access to current record has not been marked yet!");

    // Advance previous reference first, so that we maintain the access mark on the current one.
    // This ensures (due to how record removal is implemented)
    // that the current record cannot be fully removed from the list,
    // so we can use its next link to continue the traversal.
    context.pPreviousRecord = context.pCurrentRecord;
    context.autoAccessPreviousRecord.MarkAccess(&context.pPreviousRecord->accessControl);

    // Keep trying to advance current record until we succeed.
    while (!TryToAdvanceCurrentRecord(context))
    {
    }

    // Return whether we have found one more record or the end of the list.
    return (context.pCurrentRecord != nullptr);
}

void CBaseMemoryManager::ResetCurrent(IterationContext& context) const
{
    retail_assert(context.pPreviousRecord != nullptr, "Previous record cannot be null!");
    retail_assert(context.autoAccessPreviousRecord.HasMarkedAccess(), "Access to previous record has not been marked yet!");
    retail_assert(context.autoAccessCurrentRecord.HasMarkedAccess(), "Access to current record has not been marked yet!");

    context.autoAccessCurrentRecord.ReleaseAccess();

    // Keep trying to advance current record until we succeed.
    while (!TryToAdvanceCurrentRecord(context))
    {
    }
}

bool CBaseMemoryManager::TryToLockAccess(IterationContext& context, EAccessLockType wantedAccess, EAccessLockType& existingAccess) const
{
    retail_assert(wantedAccess != alt_Remove, "A Remove lock should never be taken on previous record - use UpdateRemove instead!");
    retail_assert(context.pPreviousRecord != nullptr, "Previous record cannot be null!");
    retail_assert(context.autoAccessPreviousRecord.HasMarkedAccess(), "Access to previous record has not been marked yet!");
    retail_assert(!context.autoAccessPreviousRecord.HasLockedAccess(), "Access to previous record has been locked already!");

    if (!context.autoAccessPreviousRecord.TryToLockAccess(wantedAccess, existingAccess))
    {
        return false;
    }

    // Operations other than Insert need to work with the current record,
    // so we need to check if the link between the two records still holds.
    // If the link is broken, there is no point in maintaining the lock further,
    // but we'll leave the access mark because that is managed by our caller.
    if (wantedAccess != alt_Insert
        && context.pCurrentRecord != ReadMemoryRecord(context.pPreviousRecord->next))
    {
        context.autoAccessPreviousRecord.ReleaseAccessLock();
        return false;
    }

    return true;
}

bool CBaseMemoryManager::TryToLockAccess(IterationContext& context, EAccessLockType wantedAccess) const
{
    EAccessLockType existingAccess;

    return TryToLockAccess(context, wantedAccess, existingAccess);
}


void CBaseMemoryManager::Remove(IterationContext& context) const
{
    retail_assert(context.pPreviousRecord != nullptr, "Previous record cannot be null!");
    retail_assert(context.pCurrentRecord != nullptr, "Current record cannot be null!");
    retail_assert(context.autoAccessPreviousRecord.HasLockedAccess(), "Access to previous record has not been locked yet!");
    retail_assert(context.autoAccessCurrentRecord.HasLockedAccess(), "Access to current record has not been locked yet!");
    retail_assert(
        context.pCurrentRecord == ReadMemoryRecord(context.pPreviousRecord->next),
        "Previous record pointer does not precede the current record pointer!");

    // Remove the current record from the list.
    context.pPreviousRecord->next = context.pCurrentRecord->next;

    // Wait for all other readers of current record to move away from it.
    // No new readers can appear, because we removed the record from the list.
    // This wait, coupled with the lock maintained on the previous record,
    // guarantees that pCurrentRecord->next can be safely followed by any suspended readers.
    while (context.pCurrentRecord->accessControl.readersCount > 1)
    {
        // Give other threads a chance to release read marks.
        usleep(1);
    }

    // We can now suppress the outgoing link.
    context.pCurrentRecord->next = 0;

    // We're done! Release all access immediately.
    context.autoAccessPreviousRecord.ReleaseAccess();
    context.autoAccessCurrentRecord.ReleaseAccess();
}

void CBaseMemoryManager::Insert(IterationContext& context, MemoryRecord*& pRecord) const
{
    retail_assert(context.pPreviousRecord != nullptr, "Record cannot be null!");
    retail_assert(context.autoAccessPreviousRecord.HasLockedAccess(), "Access to previous record has not been locked yet!");

    pRecord->next = context.pPreviousRecord->next;

    uint8_t* pRecordAddress = reinterpret_cast<uint8_t*>(pRecord);
    ADDRESS_OFFSET recordOffset = GetOffset(pRecordAddress);

    context.pPreviousRecord->next = recordOffset;

    // We're done! Release all access immediately.
    context.autoAccessPreviousRecord.ReleaseAccess();
}

void CBaseMemoryManager::InsertMemoryRecord(MemoryRecord* pListHead, MemoryRecord* pMemoryRecord, bool sortByOffset) const
{
    retail_assert(pListHead != nullptr, "Null list head was passed to InsertMemoryRecord()!");
    retail_assert(pMemoryRecord != nullptr, "InsertMemoryRecord() was called with a null parameter!");

    // We need an extra loop to restart the insertion from scratch in a special case.
    while (true)
    {
        IterationContext context;
        Start(pListHead, context);
        bool foundInsertionPlace = false;
        EAccessLockType existingAccess = alt_None;

        // Insert free memory record in its proper place in the free memory list,
        // based on either its size or offset value, as requested by the caller.
        while (true)
        {
            // Check if this is the right place to insert our record.
            // If we reached the end of the list, then that is the right place.
            if (context.pCurrentRecord == nullptr
                || (sortByOffset
                    ? context.pCurrentRecord->memoryOffset > pMemoryRecord->memoryOffset
                    : context.pCurrentRecord->memorySize > pMemoryRecord->memorySize))
            {
                foundInsertionPlace = true;

                if (TryToLockAccess(context, alt_Insert, existingAccess))
                {
                    Insert(context, pMemoryRecord);

                    // We're done!
                    return;
                }
                else if (existingAccess == alt_Remove)
                {
                    // We have to restart from the beginning of the list
                    // because our previous record is being deleted.
                    break;
                }
                else
                {
                    // Some other operation is taking place,
                    // so we'll have to re-examine if this is the right place to insert.
                    foundInsertionPlace = false;
                    usleep(1);
                    ResetCurrent(context);
                    continue;
                }
            }

            retail_assert(!foundInsertionPlace, "We should not reach this code if we already found the insertion place!");

            MoveNext(context);
        }
    }
}
