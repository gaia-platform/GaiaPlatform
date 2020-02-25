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

memory_manager_t::memory_manager_t() : base_memory_manager_t()
{
    m_metadata = nullptr;

    // Sanity check.
    const size_t expected_metadata_size_in_bytes = 112;
    std::stringstream message_stream;

    message_stream
     << "Metadata information structure representation does not have the expected size on this system: "
     << sizeof(metadata_t) << "!";

    retail_assert(
        sizeof(metadata_t) == expected_metadata_size_in_bytes,
        message_stream.str());
}

void memory_manager_t::set_execution_flags(const execution_flags_t& execution_flags)
{
    m_execution_flags = execution_flags;
}

error_code_t memory_manager_t::manage(
    uint8_t* memory_address,
    size_t memory_size,
    size_t main_memory_system_reserved_size,
    bool initialize)
{
    // Sanity checks.
    if (memory_address == nullptr || memory_size == 0)
    {
        return invalid_argument_value;
    }

    if (!validate_address_alignment(memory_address))
    {
        return memory_address_not_aligned;
    }

    if (!validate_size_alignment(memory_size))
    {
        return memory_size_not_aligned;
    }

    if (memory_size < sizeof(metadata_t) + main_memory_system_reserved_size
        || sizeof(metadata_t) + main_memory_system_reserved_size < main_memory_system_reserved_size)
    {
        return insufficient_memory_size;
    }

    // Save our parameters.
    m_base_memory_address = memory_address;
    m_total_memory_size = memory_size;
    m_main_memory_system_reserved_size = main_memory_system_reserved_size;

    // Map the metadata information for quick reference.
    m_metadata = reinterpret_cast<metadata_t*>(m_base_memory_address);

    // If necessary, initialize our metadata.
    if (initialize)
    {
        m_metadata->clear(
            sizeof(metadata_t),
            m_total_memory_size);
    }

    if (m_execution_flags.enable_console_output)
    {
        cout << "  Configuration - main_memory_system_reserved_size = " << m_main_memory_system_reserved_size << endl;
        cout << "  Configuration - enable_extra_validations = " << m_execution_flags.enable_extra_validations << endl;

        output_debugging_information("manage");
    }

    // The master manager is the one that initializes the memory.
    m_is_master_manager = initialize;

    return success;
}

error_code_t memory_manager_t::allocate(
    size_t memory_size,
    address_offset_t& allocated_memory_offset) const
{
    allocated_memory_offset = 0;

    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    error_code_t error_code = validate_size(memory_size);
    if (error_code != success)
    {
        return error_code;
    }

    size_t size_to_allocate = memory_size + sizeof(memory_allocation_metadata_t);

    // First, attempt to reuse freed memory blocks, if possible.
    allocated_memory_offset = allocate_from_freed_memory(size_to_allocate);

    // Otherwise, fall back to allocating from our main memory block.
    if (allocated_memory_offset == 0)
    {
        allocated_memory_offset = allocate_from_main_memory(size_to_allocate);
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("allocate");
    }

    if (allocated_memory_offset == 0)
    {
        return insufficient_memory_size;
    }

    return success;
}

error_code_t memory_manager_t::create_stack_allocator(
    size_t memory_size,
    stack_allocator_t*& stack_allocator) const
{
    stack_allocator = nullptr;

    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    address_offset_t memory_offset = 0;
    error_code_t error_code = allocate(memory_size, memory_offset);
    if (error_code != success)
    {
        return error_code;
    }

    stack_allocator = new stack_allocator_t();

    stack_allocator->set_execution_flags(m_execution_flags);

    error_code = stack_allocator->initialize(m_base_memory_address, memory_offset, memory_size);
    if (error_code != success)
    {
        delete stack_allocator;
        stack_allocator = nullptr;
    }

    return error_code;
}

error_code_t memory_manager_t::commit_stack_allocator(
    stack_allocator_t* stack_allocator,
    serialization_number_t serialization_number) const
{
    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    if (stack_allocator == nullptr)
    {
        return invalid_argument_value;
    }

    // Ensure that the stack allocator memory gets reclaimed.
    unique_ptr<stack_allocator_t> unique_stack_allocator(stack_allocator);

    size_t count_allocations = stack_allocator->get_allocation_count();

    // Get metadata record for the entire stack allocator memory block.
    memory_allocation_metadata_t* first_stack_allocation_metadata = read_allocation_metadata(stack_allocator->m_base_memory_offset);
    address_offset_t first_stack_allocation_metadata_offset
        = get_offset(reinterpret_cast<uint8_t *>(first_stack_allocation_metadata));

    // Get metadata for the stack allocator.
    stack_allocator_metadata_t* stack_allocator_metadata = stack_allocator->get_metadata();
    retail_assert(stack_allocator_metadata != nullptr, "An unexpected null metadata record was retrieved!");

    // Write serialization number.
    stack_allocator_metadata->serialization_number = serialization_number;

    if (count_allocations == 0)
    {
        // Special case: all allocations have been reverted, so we need to mark the entire memory block as free.
        if (m_execution_flags.enable_extra_validations)
        {
            retail_assert(
                first_stack_allocation_metadata_offset
                == stack_allocator->m_base_memory_offset - sizeof(memory_allocation_metadata_t),
                "Allocation metadata offset does not match manually computed size!");
            retail_assert(
                first_stack_allocation_metadata->allocation_size
                == stack_allocator->m_total_memory_size + sizeof(memory_allocation_metadata_t),
                "Allocation metadata size does not match manually computed size!");
        }

        // Try to mark memory as free. This operation can only fail if we run out of memory.
        memory_record_t* free_memory_record
            = get_free_memory_record(first_stack_allocation_metadata_offset, first_stack_allocation_metadata->allocation_size);
        if (free_memory_record == nullptr)
        {
            return insufficient_memory_size;
        }
        else
        {
            insert_free_memory_record(free_memory_record);
        }
    }
    else
    {
        // Iterate over all stack_allocator_t allocations and collect old memory offsets in free memory records.
        // However, we will not insert any of these records into the free memory list
        // until we know that our processing can no longer fail.
        unique_ptr<memory_record_t*[]> free_memory_records(new memory_record_t*[count_allocations]());
        for (size_t allocation_number = 1; allocation_number <= count_allocations; allocation_number++)
        {
            stack_allocator_allocation_t* allocation_record = stack_allocator->get_allocation_record(allocation_number);
            retail_assert(allocation_record != nullptr, "An unexpected null allocation record was retrieved!");

            if (allocation_record->old_memory_offset != 0)
            {
                memory_allocation_metadata_t* allocation_metadata = read_allocation_metadata(allocation_record->old_memory_offset);
                address_offset_t allocation_metadata_offset = get_offset(reinterpret_cast<uint8_t*>(allocation_metadata));

                // Mark memory block as free.
                // If we cannot do this, then we ran out of memory;
                // in that case we'll just reclaim all records we collected so far.
                memory_record_t* free_memory_record
                    = get_free_memory_record(allocation_metadata_offset, allocation_metadata->allocation_size);
                if (free_memory_record == nullptr)
                {
                    reclaim_records(free_memory_records.get(), count_allocations);
                    return insufficient_memory_size;
                }
                else
                {
                    free_memory_records[allocation_number - 1] = free_memory_record;
                }
                
            }
        }

        // Insert metadata block in the unserialized allocation list.
        // If we fail, reclaim all the records that we have collected so far.
        // But if we succeed, then we can insert all our collected records into the list of free memory records.
        error_code_t error_code = track_stack_allocator_metadata_for_serialization(stack_allocator_metadata);
        if (error_code != success)
        {
            reclaim_records(free_memory_records.get(), count_allocations);
            return error_code;
        }
        else
        {
            insert_free_memory_records(free_memory_records.get(), count_allocations);
        }

        // Patch metadata information of the first allocation.
        // We will deallocate the unused memory after serialization,
        // because the stack allocator metadata is still needed until then
        // and is now tracked by the unserialized allocation list.
        first_stack_allocation_metadata->allocation_size
            = stack_allocator_metadata->first_allocation_size + sizeof(memory_allocation_metadata_t);
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("commit_stack_allocator");
        stack_allocator->output_debugging_information("commit_stack_allocator");
    }

    return success;
}

error_code_t memory_manager_t::get_unserialized_allocations_list_head(memory_list_node_t*& list_head) const
{
    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    // Serializing allocations should only be performed by the database engine
    // through its master manager instance.
    if (!m_is_master_manager)
    {
        return operation_available_only_to_master_manager;
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("get_unserialized_allocations_list_head");
    }

    list_head = &m_metadata->unserialized_allocations_list_head;

    return success;
}

error_code_t memory_manager_t::update_unserialized_allocations_list_head(
        address_offset_t next_unserialized_allocation_record_offset) const
{
    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    // We require an offset because the end of the list may not be safe to process,
    // due to other threads making insertions.
    if (next_unserialized_allocation_record_offset == 0)
    {
        return invalid_argument_value;
    }

    // Serializing allocations should only be performed by the database engine
    // through its master manager instance.
    if (!m_is_master_manager)
    {
        return operation_available_only_to_master_manager;
    }

    address_offset_t current_record_offset = m_metadata->unserialized_allocations_list_head.next;
    retail_assert(current_record_offset != 0, "update_unserialized_allocations_list_head() was called on an empty list!");
    retail_assert(
        current_record_offset != next_unserialized_allocation_record_offset,
        "update_unserialized_allocations_list_head() was called on an already updated list!");

    while (current_record_offset != next_unserialized_allocation_record_offset)
    {
        // Get the actual record.
        memory_record_t* current_record = base_memory_manager_t::read_memory_record(current_record_offset);

        // Get the stack_allocator_t metadata.
        address_offset_t current_metadata_offset = current_record->memory_offset;
        uint8_t* current_metadata_address = get_address(current_metadata_offset);
        stack_allocator_metadata_t* current_metadata = reinterpret_cast<stack_allocator_metadata_t*>(current_metadata_address);

        // Determine the boundaries of the memory block that we can free from the stack_allocator_t.
        address_offset_t start_memory_offset = current_metadata->next_allocation_offset;
        address_offset_t end_memory_offset = current_metadata_offset + sizeof(stack_allocator_metadata_t);
        retail_assert(validate_offset(start_memory_offset) == success, "Calculated start memory offset is invalid");
        retail_assert(validate_offset(end_memory_offset) == success, "Calculated end memory offset is invalid");

        size_t memory_size = end_memory_offset - start_memory_offset;

        // Advance current record offset to next record, to prepare for the next iteration.
        current_record_offset = current_record->next;

        // Also advance the list head to the next record.
        // This means that when the loop condition is reached,
        // the list head will already be updated to reference the next unserialized record.
        m_metadata->unserialized_allocations_list_head.next = current_record_offset;

        // We'll repurpose the current record as a free memory record,
        // so we'll update it to track the free memory block information.
        current_record->memory_offset = start_memory_offset;
        current_record->memory_size = memory_size;
        current_record->next = 0;

        // Insert the record in the free memory list.
        insert_free_memory_record(current_record);
    }

    retail_assert(
        m_metadata->unserialized_allocations_list_head.next == next_unserialized_allocation_record_offset,
        "update_unserialized_allocations_list_head() has not properly updated the list!");

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("update_unserialized_allocations_list_head");
    }

    return success;
}

size_t memory_manager_t::get_main_memory_available_size(bool include_system_reserved_size) const
{
    size_t available_size = 0;

    // Ignore return value; we just want the available size.
    is_main_memory_exhausted(
        m_metadata->start_main_available_memory,
        m_metadata->lowest_metadata_memory_use,
        include_system_reserved_size,
        available_size);

    return available_size;
}

bool memory_manager_t::is_main_memory_exhausted(
    address_offset_t start_memory_offset,
    address_offset_t end_memory_offset,
    bool include_system_reserved_size) const
{
    size_t available_size = 0;

    return is_main_memory_exhausted(
        start_memory_offset,
        end_memory_offset,
        include_system_reserved_size,
        available_size);
}

bool memory_manager_t::is_main_memory_exhausted(
    address_offset_t start_memory_offset,
    address_offset_t end_memory_offset,
    bool include_system_reserved_size,
    size_t& available_size) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    available_size = 0;

    size_t reserved_size = include_system_reserved_size ? 0 : m_main_memory_system_reserved_size;

    if (start_memory_offset + reserved_size > end_memory_offset
        || start_memory_offset + reserved_size < start_memory_offset)
    {
        return true;
    }

    available_size = end_memory_offset - start_memory_offset - reserved_size;

    return false;
}

address_offset_t memory_manager_t::process_allocation(address_offset_t allocation_offset, size_t size_to_allocate) const
{
    retail_assert(allocation_offset != 0, "process_allocation() was called for an empty allocation!");

    // Write the allocation metadata.
    uint8_t* allocation_metadata_address = get_address(allocation_offset);
    memory_allocation_metadata_t* allocation_metadata
        = reinterpret_cast<memory_allocation_metadata_t*>(allocation_metadata_address);
    allocation_metadata->allocation_size = size_to_allocate;

    // We return the offset past the metadata.
    allocation_offset += sizeof(memory_allocation_metadata_t);
    return allocation_offset;
}

address_offset_t memory_manager_t::allocate_from_main_memory(size_t size_to_allocate) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    // Main memory allocations should not use the reserved space.
    bool include_system_reserved_size = false;

    // If the allocation exhausts our memory, we cannot perform it.
    if (get_main_memory_available_size(include_system_reserved_size) < size_to_allocate)
    {
        return 0;
    }

    // Claim the space.
    address_offset_t old_start_main_available_memory = __sync_fetch_and_add(
        &m_metadata->start_main_available_memory,
        size_to_allocate);
    address_offset_t new_start_main_available_memory = old_start_main_available_memory + size_to_allocate;

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (is_main_memory_exhausted(
        new_start_main_available_memory,
        m_metadata->lowest_metadata_memory_use,
        include_system_reserved_size))
    {
        // We exhausted the main memory so we must undo our update.
        while (!__sync_bool_compare_and_swap(
            &m_metadata->start_main_available_memory,
            new_start_main_available_memory,
            old_start_main_available_memory))
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
    address_offset_t allocation_offset = old_start_main_available_memory;
    address_offset_t adjusted_allocation_offset = process_allocation(allocation_offset, size_to_allocate);

    if (m_execution_flags.enable_console_output)
    {
        cout << endl << "Allocated " << size_to_allocate << " bytes at offset " << allocation_offset;
        cout << " from main memory." << endl;
    }

    return adjusted_allocation_offset;
}

address_offset_t memory_manager_t::allocate_from_freed_memory(size_t size_to_allocate) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    iteration_context_t context;
    start(&m_metadata->free_memory_list_head, context);
    address_offset_t allocation_offset = 0;

    // Iterate over the free memory list and try to find a large enough block for this allocation.
    while (context.current_record != nullptr)
    {
        if (context.current_record->memory_size >= size_to_allocate)
        {
            // We have 2 situations: the memory block is exactly our required size or it is larger.
            // In both cases, we need to check the size of the block again after acquiring the locks,
            // because another thread may have managed to update it before we could lock it. 
            if (context.current_record->memory_size == size_to_allocate)
            {
                if (try_to_lock_access(context, access_lock_type_t::update_remove)
                    && context.auto_access_current_record.try_to_lock_access(access_lock_type_t::remove)
                    && context.current_record->memory_size == size_to_allocate)
                {

                    retail_assert(
                        context.current_record->memory_size == size_to_allocate,
                        "Internal error - we are attempting to allocate from a block that is too small!");

                    // Jolly good! We've found a memory block of exactly the size that we were looking for.
                    remove(context);

                    allocation_offset = context.current_record->memory_offset;

                    // Reclaim the memory record for later reuse;
                    insert_reclaimed_memory_record(context.current_record);

                    break;
                }
            }
            else
            {
                if (try_to_lock_access(context, access_lock_type_t::update)
                    && context.current_record->memory_size > size_to_allocate)
                {
                    retail_assert(
                        context.current_record->memory_size > size_to_allocate,
                        "Internal error - we are attempting to allocate from a block that is too small!");

                    allocation_offset = context.current_record->memory_offset;

                    // We have more space than we needed, so update the record
                    // to track the remaining space.
                    context.current_record->memory_offset += size_to_allocate;
                    context.current_record->memory_size -= size_to_allocate;

                    break;
                }
            }
        }

        move_next(context);
    }

    if (allocation_offset == 0)
    {
        return 0;
    }

    address_offset_t adjusted_allocation_offset = process_allocation(allocation_offset, size_to_allocate);

    if (m_execution_flags.enable_console_output)
    {
        cout << endl << "Allocated " << size_to_allocate << " bytes at offset " << allocation_offset;
        cout << " from freed memory." << endl;
    }

    return adjusted_allocation_offset;
}

memory_record_t* memory_manager_t::get_memory_record() const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    memory_record_t* free_memory_record = get_reclaimed_memory_record();

    if (free_memory_record == nullptr)
    {
        free_memory_record = get_new_memory_record();
    }

    // The record should not indicate any access.
    retail_assert(
        free_memory_record == nullptr
        || free_memory_record->access_control.readers_count == 0,
        "The readers count of a new memory record should be 0!");
    retail_assert(
        free_memory_record == nullptr
        || free_memory_record->access_control.access_lock == access_lock_type_t::none,
        "The access lock of a new memory record should be none!");

    return free_memory_record;
}

memory_record_t* memory_manager_t::get_new_memory_record() const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    // Metadata allocations can use the reserved space.
    bool include_system_reserved_size = true;

    // If the allocation exhausts our memory, we cannot perform it.
    if (get_main_memory_available_size(include_system_reserved_size) < sizeof(memory_record_t))
    {
        return nullptr;
    }

    // Claim the space.
    address_offset_t old_lowest_metadata_memory_use = __sync_fetch_and_sub(
        &m_metadata->lowest_metadata_memory_use,
        sizeof(memory_record_t));
    address_offset_t new_lowest_metadata_memory_use = old_lowest_metadata_memory_use - sizeof(memory_record_t);

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (is_main_memory_exhausted(
        m_metadata->start_main_available_memory,
        new_lowest_metadata_memory_use,
        include_system_reserved_size))
    {
        // We exhausted the main memory.
        // Attempting to revert the update to m_metadata->lowest_metadata_memory_use
        // can conflict with attempts to revert updates to m_metadata->start_main_available_memory,
        // so just leave it as is (yes, we'll potentially lose a record's chunk of memory!)
        // and fail the record allocation.
        return nullptr;
    }

    // Our allocation has succeeded.
    address_offset_t record_offset = new_lowest_metadata_memory_use;

    if (m_execution_flags.enable_console_output)
    {
        cout << endl << "Allocated offset " << record_offset << " for a new memory record." << endl;
    }

    memory_record_t* free_memory_record = base_memory_manager_t::read_memory_record(record_offset);

    // This is uninitialized memory, so we need to explicitly clear it.
    free_memory_record->clear();

    return free_memory_record;
}

memory_record_t* memory_manager_t::get_reclaimed_memory_record() const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    iteration_context_t context;
    start(&m_metadata->reclaimed_records_list_head, context);
    memory_record_t* reclaimed_record = nullptr;

    // Iterate through the list of reclaimed records and attempt to extract one.
    while (context.current_record != nullptr)
    {
        // Retrieving a node from this list is going to be a small-effort initiative.
        // If we fail, we'll just allocate a new node, so eventually,
        // enough nodes will get inserted into this list that we'll succeed easily to remove one.
        //
        // We'll try to lock each node one after the other.
        if (try_to_lock_access(context, access_lock_type_t::update_remove)
            && context.auto_access_current_record.try_to_lock_access(access_lock_type_t::remove))
        {
            remove(context);

            reclaimed_record = context.current_record;

            break;
        }

        move_next(context);
    }

    if (reclaimed_record != nullptr
        && m_execution_flags.enable_console_output)
    {
        uint8_t* reclaimed_record_address = reinterpret_cast<uint8_t*>(reclaimed_record);
        address_offset_t reclaimed_record_offset = get_offset(reclaimed_record_address);

        cout << endl << "Reclaimed memory record from offset " << reclaimed_record_offset << "." << endl;
    }

    return reclaimed_record;
}

void memory_manager_t::insert_free_memory_record(memory_record_t* free_memory_record) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");
    retail_assert(free_memory_record != nullptr, "InsertFreeMemoryRecord() was called with a null parameter!");

    bool sort_by_offset = true;
    insert_memory_record(&m_metadata->free_memory_list_head, free_memory_record, sort_by_offset);
}

void memory_manager_t::insert_reclaimed_memory_record(memory_record_t* reclaimed_memory_record) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");
    retail_assert(reclaimed_memory_record != nullptr, "insert_reclaimed_memory_record() was called with a null parameter!");

    iteration_context_t context;
    start(&m_metadata->reclaimed_records_list_head, context);

    // We'll keep trying to insert at the beginning of the list.
    while (true)
    {
        if (try_to_lock_access(context, access_lock_type_t::insert))
        {
            insert(context, reclaimed_memory_record);

            return;
        }
    }
}

void memory_manager_t::insert_unserialized_allocations_record(memory_record_t* pUnserializedAllocationsRecord) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");
    retail_assert(pUnserializedAllocationsRecord != nullptr, "insert_unserialized_allocations_record() was called with a null parameter!");

    // In this case we want to sort by the size value, which holds the serialization number.
    bool sort_by_offset = false;
    insert_memory_record(&m_metadata->unserialized_allocations_list_head, pUnserializedAllocationsRecord, sort_by_offset);
}

memory_record_t* memory_manager_t::get_free_memory_record(address_offset_t memory_offset, size_t memory_size) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    memory_record_t* free_memory_record = get_memory_record();

    if (free_memory_record == nullptr)
    {
        return nullptr;
    }

    free_memory_record->memory_offset = memory_offset;
    free_memory_record->memory_size = memory_size;

    return free_memory_record;
}

void memory_manager_t::process_free_memory_records(memory_record_t** free_memory_records, size_t size, bool mark_as_free) const
{
    retail_assert(free_memory_records != nullptr, "process_free_memory_records() has been called with a null parameter!");

    for (size_t i = 0; i < size; i++)
    {
        if (free_memory_records[i] != nullptr)
        {
            if (mark_as_free)
            {
                insert_free_memory_record(free_memory_records[i]);
            }
            else
            {
                insert_reclaimed_memory_record(free_memory_records[i]);
            }
        }
    }
}

void memory_manager_t::insert_free_memory_records(memory_record_t** free_memory_records, size_t size) const
{
    return process_free_memory_records(free_memory_records, size, true);
}

void memory_manager_t::reclaim_records(memory_record_t** free_memory_records, size_t size) const
{
    return process_free_memory_records(free_memory_records, size, false);
}

error_code_t memory_manager_t::track_stack_allocator_metadata_for_serialization(
    stack_allocator_metadata_t* stack_allocator_metadata) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    memory_record_t* memory_record = get_memory_record();

    if (memory_record == nullptr)
    {
        return insufficient_memory_size;
    }

    uint8_t* metadata_address = reinterpret_cast<uint8_t*>(stack_allocator_metadata);
    address_offset_t metadata_offset = get_offset(metadata_address);

    memory_record->memory_offset = metadata_offset;

    // We don't need this field for its original purpose,
    // so we'll repurpose it to hold the serialization number,
    // to make it easier to check this value during list insertions.
    memory_record->memory_size = stack_allocator_metadata->serialization_number;

    insert_unserialized_allocations_record(memory_record);

    return success;
}

void memory_manager_t::output_debugging_information(const string& context_description) const
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "Debugging output for context: " << context_description << ":" << endl;

    if (m_metadata == nullptr)
    {
        cout << "  Memory manager has not been initialized." << endl;
    }

    cout << "  Main memory start = " << m_metadata->start_main_available_memory << endl;
    cout << "  Metadata start = " << m_metadata->lowest_metadata_memory_use << endl;
    cout << "  Available main memory including reserved space = " << get_main_memory_available_size(true) << endl;
    cout << "  Available main memory excluding reserved space = " << get_main_memory_available_size(false) << endl;
    cout << "  Free memory list: " << endl;
    output_list_content(m_metadata->free_memory_list_head);
    cout << "  Reclaimed records list: " << endl;
    output_list_content(m_metadata->reclaimed_records_list_head); 
    cout << "  Unserialized allocations list: " << endl;
    output_list_content(m_metadata->unserialized_allocations_list_head);
    cout << c_debug_output_separator_line_end << endl;
}

void memory_manager_t::output_list_content(memory_record_t list_head) const
{
    size_t record_count = 0;
    address_offset_t current_record_offset = list_head.next;
    while (current_record_offset != 0)
    {
        record_count++;

        memory_record_t* current_record = read_memory_record(current_record_offset);

        cout << "    Record[" << record_count << "] at offset " << current_record_offset << ":" << endl;
        cout << "      offset = " << current_record->memory_offset;
        cout << " size = " << current_record->memory_size;
        cout << " readersCount = " << current_record->access_control.readers_count;
        cout << " accessLock = " << current_record->access_control.access_lock;
        cout << " next = " << current_record->next << endl;

        current_record_offset = current_record->next;
    }

    cout << "    List contains " << record_count << " records." << endl;
}
