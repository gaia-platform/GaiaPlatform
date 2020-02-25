/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include "retail_assert.hpp"
#include "base_memory_manager.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;

base_memory_manager::base_memory_manager()
{
    m_base_memory_address = nullptr;
    m_base_memory_offset = 0;
    m_total_memory_size = 0;
}

bool base_memory_manager::validate_address_alignment(const uint8_t* const pMemoryAddress) const
{
    size_t memoryAddressAsInteger = reinterpret_cast<size_t>(pMemoryAddress);
    return (memoryAddressAsInteger % c_memory_alignment == 0);
}

bool base_memory_manager::validate_offset_alignment(ADDRESS_OFFSET memoryOffset) const
{
    return (memoryOffset % c_memory_alignment == 0);
}

bool base_memory_manager::validate_size_alignment(size_t memorySize) const
{
    return (memorySize % c_memory_alignment == 0);
}

gaia::db::memory_manager::error_code base_memory_manager::validate_address(const uint8_t* const pMemoryAddress) const
{
    if (!validate_address_alignment(pMemoryAddress))
    {
        return memory_address_not_aligned;
    }

    if (pMemoryAddress < m_base_memory_address + m_base_memory_offset
        || pMemoryAddress > m_base_memory_address + m_base_memory_offset + m_total_memory_size)
    {
        return memory_address_out_of_range;
    }

    return success;
}

gaia::db::memory_manager::error_code base_memory_manager::validate_offset(ADDRESS_OFFSET memoryOffset) const
{
    if (!validate_offset_alignment(memoryOffset))
    {
        return memory_offset_not_aligned;
    }

    if (memoryOffset < m_base_memory_offset
        || memoryOffset > m_base_memory_offset + m_total_memory_size)
    {
        return memory_offset_out_of_range;
    }

    return success;
}

gaia::db::memory_manager::error_code base_memory_manager::validate_size(size_t memorySize) const
{
    if (!validate_size_alignment(memorySize))
    {
        return memory_size_not_aligned;
    }

    if (memorySize == 0)
    {
        return memory_size_cannot_be_zero;
    }

    if (memorySize > m_total_memory_size)
    {
        return memory_size_too_large;
    }

    return success;
}

ADDRESS_OFFSET base_memory_manager::get_offset(const uint8_t* const pMemoryAddress) const
{
    retail_assert(
        validate_address(pMemoryAddress) == success,
        "GetOffset() was called with an invalid address!");

    size_t offset = pMemoryAddress - m_base_memory_address;

    return offset;
}

uint8_t* base_memory_manager::get_address(ADDRESS_OFFSET memoryOffset) const
{
    retail_assert(
        validate_offset(memoryOffset) == success,
        "GetAddress() was called with an invalid offset!");

    uint8_t* pMemoryAddress = m_base_memory_address + memoryOffset;

    return pMemoryAddress;
}

memory_allocation_metadata* base_memory_manager::read_allocation_metadata(ADDRESS_OFFSET memoryOffset) const
{
    retail_assert(
        validate_offset(memoryOffset) == success,
        "GetAllocationMetadata() was called with an invalid offset!");

    retail_assert(
        memoryOffset >= sizeof(memory_allocation_metadata),
        "GetAllocationMetadata() was called with an offset that is too small!");

    ADDRESS_OFFSET allocationMetadataOffset = memoryOffset - sizeof(memory_allocation_metadata);
    uint8_t* pAllocationMetadataAddress = get_address(allocationMetadataOffset);
    memory_allocation_metadata* pAllocationMetadata
        = reinterpret_cast<memory_allocation_metadata*>(pAllocationMetadataAddress);

    return pAllocationMetadata;
}

memory_record* base_memory_manager::read_memory_record(ADDRESS_OFFSET recordOffset) const
{
    retail_assert(
        validate_offset(recordOffset) == success,
        "GetFreeMemoryRecord was called with an invalid offset!");

    retail_assert(
        recordOffset > 0,
        "GetFreeMemoryRecord was called with a zero offset!");

    uint8_t* pRecordAddress = get_address(recordOffset);
    memory_record* pRecord = reinterpret_cast<memory_record*>(pRecordAddress);

    return pRecord;
}

bool base_memory_manager::try_to_advance_current_record(iteration_context& context) const
{
    retail_assert(context.previous_record != nullptr, "Previous record cannot be null!");

    ADDRESS_OFFSET currentRecordOffset = context.previous_record->next;

    if (currentRecordOffset == 0)
    {
        context.current_record = nullptr;
        context.auto_access_current_record.release_access();

        return true;
    }

    context.current_record = read_memory_record(currentRecordOffset);
    context.auto_access_current_record.mark_access(&context.current_record->accessControl);

    // Now that we marked our access, we need to check
    // whether pCurrentRecord still belongs in our list,
    // because another thread may have removed it
    // before we got to mark our access.
    if (context.previous_record->next == currentRecordOffset)
    {
        // All looks good, so we're done here.
        return true;
    }

    // We failed, so we should release the access that we marked.
    context.auto_access_current_record.release_access();

    return false;
}

void base_memory_manager::start(memory_record* pListHead, iteration_context& context) const
{
    retail_assert(pListHead != nullptr, "Null list head was passed to CBaseMemoryManager::Start()!");

    context.previous_record = pListHead;
    context.auto_access_previous_record.mark_access(&context.previous_record->accessControl);

    // Keep trying to advance current record until we succeed.
    while (!try_to_advance_current_record(context))
    {
    }
}

bool base_memory_manager::move_next(iteration_context& context) const
{
    retail_assert(context.previous_record != nullptr, "Previous record cannot be null!");
    retail_assert(context.auto_access_previous_record.has_marked_access(), "Access to previous record has not been marked yet!");

    if (context.current_record == nullptr)
    {
        return false;
    }

    retail_assert(context.auto_access_current_record.has_marked_access(), "Access to current record has not been marked yet!");

    // Advance previous reference first, so that we maintain the access mark on the current one.
    // This ensures (due to how record removal is implemented)
    // that the current record cannot be fully removed from the list,
    // so we can use its next link to continue the traversal.
    context.previous_record = context.current_record;
    context.auto_access_previous_record.mark_access(&context.previous_record->accessControl);

    // Keep trying to advance current record until we succeed.
    while (!try_to_advance_current_record(context))
    {
    }

    // Return whether we have found one more record or the end of the list.
    return (context.current_record != nullptr);
}

void base_memory_manager::reset_current(iteration_context& context) const
{
    retail_assert(context.previous_record != nullptr, "Previous record cannot be null!");
    retail_assert(context.auto_access_previous_record.has_marked_access(), "Access to previous record has not been marked yet!");
    retail_assert(context.auto_access_current_record.has_marked_access(), "Access to current record has not been marked yet!");

    context.auto_access_current_record.release_access();

    // Keep trying to advance current record until we succeed.
    while (!try_to_advance_current_record(context))
    {
    }
}

bool base_memory_manager::try_to_lock_access(iteration_context& context, access_lock_type wantedAccess, access_lock_type& existingAccess) const
{
    retail_assert(wantedAccess != access_lock_type::remove, "A Remove lock should never be taken on previous record - use UpdateRemove instead!");
    retail_assert(context.previous_record != nullptr, "Previous record cannot be null!");
    retail_assert(context.auto_access_previous_record.has_marked_access(), "Access to previous record has not been marked yet!");
    retail_assert(!context.auto_access_previous_record.has_locked_access(), "Access to previous record has been locked already!");

    if (!context.auto_access_previous_record.try_to_lock_access(wantedAccess, existingAccess))
    {
        return false;
    }

    // Operations other than Insert need to work with the current record,
    // so we need to check if the link between the two records still holds.
    // If the link is broken, there is no point in maintaining the lock further,
    // but we'll leave the access mark because that is managed by our caller.
    if (wantedAccess != access_lock_type::insert
        && context.current_record != read_memory_record(context.previous_record->next))
    {
        context.auto_access_previous_record.release_access_lock();
        return false;
    }

    return true;
}

bool base_memory_manager::try_to_lock_access(iteration_context& context, access_lock_type wantedAccess) const
{
    access_lock_type existingAccess;

    return try_to_lock_access(context, wantedAccess, existingAccess);
}


void base_memory_manager::remove(iteration_context& context) const
{
    retail_assert(context.previous_record != nullptr, "Previous record cannot be null!");
    retail_assert(context.current_record != nullptr, "Current record cannot be null!");
    retail_assert(context.auto_access_previous_record.has_locked_access(), "Access to previous record has not been locked yet!");
    retail_assert(context.auto_access_current_record.has_locked_access(), "Access to current record has not been locked yet!");
    retail_assert(
        context.current_record == read_memory_record(context.previous_record->next),
        "Previous record pointer does not precede the current record pointer!");

    // Remove the current record from the list.
    context.previous_record->next = context.current_record->next;

    // Wait for all other readers of current record to move away from it.
    // No new readers can appear, because we removed the record from the list.
    // This wait, coupled with the lock maintained on the previous record,
    // guarantees that pCurrentRecord->next can be safely followed by any suspended readers.
    while (context.current_record->accessControl.readers_count > 1)
    {
        // Give other threads a chance to release read marks.
        usleep(1);
    }

    // We can now suppress the outgoing link.
    context.current_record->next = 0;

    // We're done! Release all access immediately.
    context.auto_access_previous_record.release_access();
    context.auto_access_current_record.release_access();
}

void base_memory_manager::insert(iteration_context& context, memory_record*& pRecord) const
{
    retail_assert(context.previous_record != nullptr, "Record cannot be null!");
    retail_assert(context.auto_access_previous_record.has_locked_access(), "Access to previous record has not been locked yet!");

    pRecord->next = context.previous_record->next;

    uint8_t* pRecordAddress = reinterpret_cast<uint8_t*>(pRecord);
    ADDRESS_OFFSET recordOffset = get_offset(pRecordAddress);

    context.previous_record->next = recordOffset;

    // We're done! Release all access immediately.
    context.auto_access_previous_record.release_access();
}

void base_memory_manager::insert_memory_record(memory_record* pListHead, memory_record* pMemoryRecord, bool sortByOffset) const
{
    retail_assert(pListHead != nullptr, "Null list head was passed to InsertMemoryRecord()!");
    retail_assert(pMemoryRecord != nullptr, "InsertMemoryRecord() was called with a null parameter!");

    // We need an extra loop to restart the insertion from scratch in a special case.
    while (true)
    {
        iteration_context context;
        start(pListHead, context);
        bool foundInsertionPlace = false;
        access_lock_type existingAccess = access_lock_type::none;

        // Insert free memory record in its proper place in the free memory list,
        // based on either its size or offset value, as requested by the caller.
        while (true)
        {
            // Check if this is the right place to insert our record.
            // If we reached the end of the list, then that is the right place.
            if (context.current_record == nullptr
                || (sortByOffset
                    ? context.current_record->memory_offset > pMemoryRecord->memory_offset
                    : context.current_record->memory_size > pMemoryRecord->memory_size))
            {
                foundInsertionPlace = true;

                if (try_to_lock_access(context, access_lock_type::insert, existingAccess))
                {
                    insert(context, pMemoryRecord);

                    // We're done!
                    return;
                }
                else if (existingAccess == access_lock_type::remove)
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
                    reset_current(context);
                    continue;
                }
            }

            retail_assert(!foundInsertionPlace, "We should not reach this code if we already found the insertion place!");

            move_next(context);
        }
    }
}
