/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "base_memory_manager.hpp"

#include <unistd.h>

#include "retail_assert.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;

base_memory_manager_t::base_memory_manager_t()
{
    m_base_memory_address = nullptr;
    m_base_memory_offset = 0;
    m_total_memory_size = 0;
}

bool base_memory_manager_t::validate_address_alignment(const uint8_t* const memory_address) const
{
    size_t memoryAddressAsInteger = reinterpret_cast<size_t>(memory_address);
    return (memoryAddressAsInteger % c_memory_alignment == 0);
}

bool base_memory_manager_t::validate_offset_alignment(address_offset_t memory_offset) const
{
    return (memory_offset % c_memory_alignment == 0);
}

bool base_memory_manager_t::validate_size_alignment(size_t memory_size) const
{
    return (memory_size % c_memory_alignment == 0);
}

error_code_t base_memory_manager_t::validate_address(const uint8_t* const memory_address) const
{
    if (!validate_address_alignment(memory_address))
    {
        return error_code_t::memory_address_not_aligned;
    }

    if (memory_address < m_base_memory_address + m_base_memory_offset
        || memory_address > m_base_memory_address + m_base_memory_offset + m_total_memory_size)
    {
        return error_code_t::memory_address_out_of_range;
    }

    return error_code_t::success;
}

error_code_t base_memory_manager_t::validate_offset(address_offset_t memory_offset) const
{
    if (!validate_offset_alignment(memory_offset))
    {
        return error_code_t::memory_offset_not_aligned;
    }

    if (memory_offset < m_base_memory_offset
        || memory_offset > m_base_memory_offset + m_total_memory_size)
    {
        return error_code_t::memory_offset_out_of_range;
    }

    return error_code_t::success;
}

error_code_t base_memory_manager_t::validate_size(size_t memory_size) const
{
    if (!validate_size_alignment(memory_size))
    {
        return error_code_t::memory_size_not_aligned;
    }

    if (memory_size == 0)
    {
        return error_code_t::memory_size_cannot_be_zero;
    }

    if (memory_size > m_total_memory_size)
    {
        return error_code_t::memory_size_too_large;
    }

    return error_code_t::success;
}

address_offset_t base_memory_manager_t::get_offset(const uint8_t* const memory_address) const
{
    retail_assert(
        validate_address(memory_address) == error_code_t::success,
        "get_offset() was called with an invalid address!");

    size_t memory_offset = memory_address - m_base_memory_address;

    return memory_offset;
}

uint8_t* base_memory_manager_t::get_address(address_offset_t memory_offset) const
{
    retail_assert(
        validate_offset(memory_offset) == error_code_t::success,
        "get_address() was called with an invalid offset!");

    uint8_t* memory_address = m_base_memory_address + memory_offset;

    return memory_address;
}

memory_allocation_metadata_t* base_memory_manager_t::read_allocation_metadata(address_offset_t memory_offset) const
{
    retail_assert(
        validate_offset(memory_offset) == error_code_t::success,
        "read_allocation_metadata() was called with an invalid offset!");

    retail_assert(
        memory_offset >= sizeof(memory_allocation_metadata_t),
        "read_allocation_metadata() was called with an offset that is too small!");

    address_offset_t allocation_metadata_offset = memory_offset - sizeof(memory_allocation_metadata_t);
    uint8_t* allocation_metadata_address = get_address(allocation_metadata_offset);
    memory_allocation_metadata_t* allocation_metadata
        = reinterpret_cast<memory_allocation_metadata_t*>(allocation_metadata_address);

    return allocation_metadata;
}

memory_record_t* base_memory_manager_t::read_memory_record(address_offset_t record_offset) const
{
    retail_assert(
        validate_offset(record_offset) == error_code_t::success,
        "read_memory_record() was called with an invalid offset!");

    retail_assert(
        record_offset > 0,
        "read_memory_record() was called with a zero offset!");

    uint8_t* record_address = get_address(record_offset);
    memory_record_t* record = reinterpret_cast<memory_record_t*>(record_address);

    return record;
}

bool base_memory_manager_t::try_to_advance_current_record(iteration_context_t& context) const
{
    retail_assert(context.previous_record != nullptr, "Previous record cannot be null!");

    address_offset_t current_record_offset = context.previous_record->next;

    if (current_record_offset == 0)
    {
        context.current_record = nullptr;
        context.auto_access_current_record.release_access();

        return true;
    }

    context.current_record = read_memory_record(current_record_offset);
    context.auto_access_current_record.mark_access(&context.current_record->access_control);

    // Now that we marked our access, we need to check
    // whether current_record still belongs in our list,
    // because another thread may have removed it
    // before we got to mark our access.
    if (context.previous_record->next == current_record_offset)
    {
        // All looks good, so we're done here.
        return true;
    }

    // We failed, so we should release the access that we marked.
    context.auto_access_current_record.release_access();

    return false;
}

void base_memory_manager_t::start(memory_record_t* list_head, iteration_context_t& context) const
{
    retail_assert(list_head != nullptr, "Null list head was passed to base_memory_manager_t::start()!");

    context.previous_record = list_head;
    context.auto_access_previous_record.mark_access(&context.previous_record->access_control);

    // Keep trying to advance current record until we succeed.
    while (!try_to_advance_current_record(context))
    {
    }
}

bool base_memory_manager_t::move_next(iteration_context_t& context) const
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
    context.auto_access_previous_record.mark_access(&context.previous_record->access_control);

    // Keep trying to advance current record until we succeed.
    while (!try_to_advance_current_record(context))
    {
    }

    // Return whether we have found one more record or the end of the list.
    return (context.current_record != nullptr);
}

void base_memory_manager_t::reset_current(iteration_context_t& context) const
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

bool base_memory_manager_t::try_to_lock_access(iteration_context_t& context, access_lock_type_t wanted_access, access_lock_type_t& existing_access) const
{
    retail_assert(wanted_access != access_lock_type_t::remove, "A remove lock should never be taken on previous record - use update_remove instead!");
    retail_assert(context.previous_record != nullptr, "Previous record cannot be null!");
    retail_assert(context.auto_access_previous_record.has_marked_access(), "Access to previous record has not been marked yet!");
    retail_assert(!context.auto_access_previous_record.has_locked_access(), "Access to previous record has been locked already!");

    if (!context.auto_access_previous_record.try_to_lock_access(wanted_access, existing_access))
    {
        return false;
    }

    // Operations other than insert need to work with the current record,
    // so we need to check if the link between the two records still holds.
    // If the link is broken, there is no point in maintaining the lock further,
    // but we'll leave the access mark because that is managed by our caller.
    if (wanted_access != access_lock_type_t::insert
        && context.current_record != read_memory_record(context.previous_record->next))
    {
        context.auto_access_previous_record.release_access_lock();
        return false;
    }

    return true;
}

bool base_memory_manager_t::try_to_lock_access(iteration_context_t& context, access_lock_type_t wanted_access) const
{
    access_lock_type_t existing_access;

    return try_to_lock_access(context, wanted_access, existing_access);
}


void base_memory_manager_t::remove(iteration_context_t& context) const
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
    // guarantees that current_record->next can be safely followed by any suspended readers.
    while (context.current_record->access_control.readers_count > 1)
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

void base_memory_manager_t::insert(iteration_context_t& context, memory_record_t*& record) const
{
    retail_assert(context.previous_record != nullptr, "Record cannot be null!");
    retail_assert(context.auto_access_previous_record.has_locked_access(), "Access to previous record has not been locked yet!");

    record->next = context.previous_record->next;

    uint8_t* record_address = reinterpret_cast<uint8_t*>(record);
    address_offset_t record_offset = get_offset(record_address);

    context.previous_record->next = record_offset;

    // We're done! Release all access immediately.
    context.auto_access_previous_record.release_access();
}

void base_memory_manager_t::insert_memory_record(memory_record_t* list_head, memory_record_t* memory_record, bool sort_by_offset) const
{
    retail_assert(list_head != nullptr, "Null list head was passed to insert_memory_record()!");
    retail_assert(memory_record != nullptr, "insert_memory_record() was called with a null parameter!");

    // We need an extra loop to restart the insertion from scratch in a special case.
    while (true)
    {
        iteration_context_t context;
        start(list_head, context);
        bool found_insertion_place = false;
        access_lock_type_t existing_access = access_lock_type_t::none;

        // Insert free memory record in its proper place in the free memory list,
        // based on either its size or offset value, as requested by the caller.
        while (true)
        {
            // Check if this is the right place to insert our record.
            // If we reached the end of the list, then that is the right place.
            if (context.current_record == nullptr
                || (sort_by_offset
                    ? context.current_record->memory_offset > memory_record->memory_offset
                    : context.current_record->memory_size > memory_record->memory_size))
            {
                found_insertion_place = true;

                if (try_to_lock_access(context, access_lock_type_t::insert, existing_access))
                {
                    insert(context, memory_record);

                    // We're done!
                    return;
                }
                else if (existing_access == access_lock_type_t::remove)
                {
                    // We have to restart from the beginning of the list
                    // because our previous record is being deleted.
                    break;
                }
                else
                {
                    // Some other operation is taking place,
                    // so we'll have to re-examine if this is the right place to insert.
                    found_insertion_place = false;
                    usleep(1);
                    reset_current(context);
                    continue;
                }
            }

            retail_assert(!found_insertion_place, "We should not reach this code if we already found the insertion place!");

            move_next(context);
        }
    }
}
