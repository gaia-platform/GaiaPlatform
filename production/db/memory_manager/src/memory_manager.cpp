/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "memory_manager.hpp"

#include <unistd.h>

#include <sstream>
#include <memory>

#include "retail_assert.hpp"

#include "types.hpp"
#include "access_control.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db::memory_manager;

memory_manager_t::memory_manager_t() : base_memory_manager_t()
{
}

error_code_t memory_manager_t::manage(
    uint8_t* memory_address,
    size_t memory_size)
{
    // Sanity checks.
    if (memory_address == nullptr || memory_size == 0)
    {
        return error_code_t::invalid_argument_value;
    }

    if (!validate_address_alignment(memory_address))
    {
        return error_code_t::memory_address_not_aligned;
    }

    if (!validate_size_alignment(memory_size))
    {
        return error_code_t::memory_size_not_aligned;
    }

    // Save our parameters.
    m_base_memory_address = memory_address;
    m_total_memory_size = memory_size;

    if (m_execution_flags.enable_console_output)
    {
        cout << "  Configuration - enable_extra_validations = " << m_execution_flags.enable_extra_validations << endl;

        output_debugging_information("manage");
    }

    return error_code_t::success;
}

error_code_t memory_manager_t::allocate(
    size_t memory_size,
    address_offset_t& allocated_memory_offset) const
{
    allocated_memory_offset = 0;

    error_code_t error_code = validate_size(memory_size);
    if (error_code != error_code_t::success)
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
        return error_code_t::insufficient_memory_size;
    }

    return error_code_t::success;
}

error_code_t memory_manager_t::commit_stack_allocator(
    unique_ptr<stack_allocator_t> stack_allocator) const
{
    if (stack_allocator == nullptr)
    {
        return error_code_t::invalid_argument_value;
    }

    size_t count_allocations = stack_allocator->get_allocation_count();

    // Get metadata record for the entire stack allocator memory block.
    memory_allocation_metadata_t* first_stack_allocation_metadata
        = read_allocation_metadata(stack_allocator->m_base_memory_offset);
    address_offset_t first_stack_allocation_metadata_offset
        = get_offset(reinterpret_cast<uint8_t *>(first_stack_allocation_metadata));

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
        unique_lock unique_free_memory_list_lock(m_free_memory_list_lock);
        m_free_memory_list.emplace_back(
            first_stack_allocation_metadata_offset,
            first_stack_allocation_metadata->allocation_size);
    }
    else
    {
        // Iterate over all stack_allocator_t allocations and collect old memory offsets in free memory records.
        for (size_t allocation_number = 1; allocation_number <= count_allocations; allocation_number++)
        {
            stack_allocator_allocation_t* allocation_record = stack_allocator->get_allocation_record(allocation_number);
            retail_assert(allocation_record != nullptr, "An unexpected null allocation record was retrieved!");

            if (allocation_record->old_memory_offset != 0)
            {
                memory_allocation_metadata_t* allocation_metadata = read_allocation_metadata(allocation_record->old_memory_offset);
                address_offset_t allocation_metadata_offset = get_offset(reinterpret_cast<uint8_t*>(allocation_metadata));

                // Add allocation to free memory block list.
                unique_lock unique_free_memory_list_lock(m_free_memory_list_lock);
                m_free_memory_list.emplace_back(
                    allocation_metadata_offset,
                    allocation_metadata->allocation_size);
            }
        }

        // Patch metadata information of the first allocation.
        first_stack_allocation_metadata->allocation_size
            = stack_allocator_metadata->first_allocation_size + sizeof(memory_allocation_metadata_t);

        // Now we need to release the unused stack allocator memory.
        // Get the stack_allocator_t metadata.
        stack_allocator_metadata_t* stack_metadata = stack_allocator->get_metadata();
        address_offset_t stack_metadata_offset = get_offset(reinterpret_cast<uint8_t*>(stack_metadata));

        // Determine the boundaries of the memory block that we can free from the stack_allocator_t.
        address_offset_t start_memory_offset = stack_metadata->next_allocation_offset;
        address_offset_t end_memory_offset = stack_metadata_offset + sizeof(stack_allocator_metadata_t);
        retail_assert(validate_offset(start_memory_offset) == error_code_t::success, "Calculated start memory offset is invalid");
        retail_assert(validate_offset(end_memory_offset) == error_code_t::success, "Calculated end memory offset is invalid");

        size_t memory_size = end_memory_offset - start_memory_offset;

        // Add block information to free memory block list.
        unique_lock unique_free_memory_list_lock(m_free_memory_list_lock);
        m_free_memory_list.emplace_back(
            start_memory_offset,
            memory_size);
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("commit_stack_allocator");
        stack_allocator->output_debugging_information("commit_stack_allocator");
    }

    return error_code_t::success;
}

size_t memory_manager_t::get_main_memory_available_size() const
{
    size_t available_size = m_base_memory_offset + m_total_memory_size - m_next_allocation_offset;

    return available_size;
}

address_offset_t memory_manager_t::process_allocation(address_offset_t allocation_offset, size_t size_to_allocate) const
{
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
    // If the allocation exhausts our memory, we cannot perform it.
    if (get_main_memory_available_size() < size_to_allocate)
    {
        return 0;
    }

    // Claim the space.
    address_offset_t old_next_allocation_offset = __sync_fetch_and_add(
        &m_next_allocation_offset,
        size_to_allocate);
    address_offset_t new_next_allocation_offset = old_next_allocation_offset + size_to_allocate;

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (new_next_allocation_offset > m_base_memory_address + m_total_memory_size)
    {
        // We exhausted the main memory so we must undo our update.
        while (!__sync_bool_compare_and_swap(
            &m_next_allocation_offset,
            new_next_allocation_offset,
            old_next_allocation_offset))
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
    address_offset_t allocation_offset = old_next_allocation_offset;
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
        cout << " readers_count = " << current_record->access_control.readers_count;
        cout << " access_lock = " << static_cast<int8_t>(current_record->access_control.access_lock);
        cout << " next = " << current_record->next << endl;

        current_record_offset = current_record->next;
    }

    cout << "    List contains " << record_count << " records." << endl;
}
