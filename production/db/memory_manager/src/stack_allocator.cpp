/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "stack_allocator.hpp"

#include "retail_assert.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;

stack_allocator_t::stack_allocator_t()
{
    m_metadata = nullptr;
}

error_code_t stack_allocator_t::initialize(
    uint8_t* base_memory_address,
    address_offset_t memory_offset,
    size_t memory_size)
{
    if (base_memory_address == nullptr || memory_offset == 0 || memory_size == 0)
    {
        return error_code_t::invalid_argument_value;
    }

    if (!validate_address_alignment(base_memory_address))
    {
        return error_code_t::memory_address_not_aligned;
    }

    if (!validate_offset_alignment(memory_offset))
    {
        return error_code_t::memory_offset_not_aligned;
    }

    if (!validate_size_alignment(memory_size))
    {
        return error_code_t::memory_size_not_aligned;
    }

    if (memory_size < sizeof(stack_allocator_metadata_t))
    {
        return error_code_t::insufficient_memory_size;
    }

    // Save our parameters.
    m_base_memory_address = base_memory_address;
    m_base_memory_offset = memory_offset;
    m_total_memory_size = memory_size;

    // Map the metadata information for quick reference.
    uint8_t* metadata_address
        = m_base_memory_address + m_base_memory_offset + m_total_memory_size - sizeof(stack_allocator_metadata_t);
    m_metadata = reinterpret_cast<stack_allocator_metadata_t*>(metadata_address);
    m_metadata->clear();
    m_metadata->next_allocation_offset = m_base_memory_offset;

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("initialize");
    }

    return error_code_t::success;
}

error_code_t stack_allocator_t::allocate(
    slot_id_t slot_id,
    address_offset_t old_slot_offset,
    size_t memory_size,
    address_offset_t& allocated_memory_offset) const
{
    allocated_memory_offset = 0;

    if (m_metadata == nullptr)
    {
        return error_code_t::not_initialized;
    }

    // A memory_size of 0 indicates a deletion - handle specially.
    error_code_t error_code = validate_size(memory_size);
    if (memory_size != 0 && error_code != error_code_t::success)
    {
        return error_code;
    }

    if (!validate_offset_alignment(old_slot_offset))
    {
        return error_code_t::memory_offset_not_aligned;
    }

    // For all but the first allocation, we will prefix a memory allocation metadata block.
    // The first allocation will eventually reuse the memory allocation metadata block
    // that is currently tracking the entire memory of the stack allocator.
    size_t size_to_allocate = memory_size;
    size_t count_allocations = m_metadata->count_allocations;
    if (size_to_allocate > 0 && count_allocations > 0)
    {
        size_to_allocate += sizeof(memory_allocation_metadata_t);
    }

    address_offset_t next_allocation_offset = m_metadata->next_allocation_offset;
    address_offset_t metadata_offset = get_offset(reinterpret_cast<uint8_t*>(m_metadata));

    // Now we can do a more accurate check about whether this allocation fits in our memory.
    if (next_allocation_offset + size_to_allocate
        > metadata_offset - (count_allocations + 1) * sizeof(stack_allocator_allocation_t))
    {
        return error_code_t::insufficient_memory_size;
    }

    if (size_to_allocate > 0)
    {
        // Perform allocation.
        if (count_allocations == 0)
        {
            m_metadata->first_allocation_size = size_to_allocate;
        }
        else
        {
            uint8_t* next_allocation_address = get_address(next_allocation_offset);
            memory_allocation_metadata_t* next_allocation_metadata
                = reinterpret_cast<memory_allocation_metadata_t*>(next_allocation_address);
            next_allocation_metadata->allocation_size = size_to_allocate;
        }
    }

    m_metadata->count_allocations++;

    // Record allocation information.
    stack_allocator_allocation_t* allocation_record = get_allocation_record(m_metadata->count_allocations);
    allocation_record->clear();

    allocation_record->slot_id = slot_id;
    allocation_record->old_memory_offset = old_slot_offset;

    // Set allocation offset.
    if (size_to_allocate > 0)
    {
        if (count_allocations == 0)
        {
            allocation_record->memory_offset = next_allocation_offset;
            retail_assert(
                next_allocation_offset == m_base_memory_offset,
                "For first allocation, the computed offset should be the base offset.");
        }
        else
        {
            allocation_record->memory_offset = next_allocation_offset + sizeof(memory_allocation_metadata_t);
        }
    }

    m_metadata->next_allocation_offset += size_to_allocate;

    if (m_execution_flags.enable_console_output)
    {
        string context = (size_to_allocate > 0) ? "allocate" : "deallocate";
        output_debugging_information(context);
    }

    allocated_memory_offset = allocation_record->memory_offset;

    return error_code_t::success;
}

error_code_t stack_allocator_t::deallocate(slot_id_t slot_id, address_offset_t slot_offset) const
{
    address_offset_t allocated_offset;

    error_code_t error_code = allocate(slot_id, slot_offset, 0, allocated_offset);

    retail_assert(allocated_offset == 0, "allocate(0) should have returned a 0 offset!");

    return error_code;
}

error_code_t stack_allocator_t::deallocate(size_t count_allocations_to_keep) const
{
    if (m_metadata == nullptr)
    {
        return error_code_t::not_initialized;
    }

    if (count_allocations_to_keep > m_metadata->count_allocations)
    {
        return error_code_t::allocation_count_too_large;
    }

    m_metadata->count_allocations = count_allocations_to_keep;

    // Reset first allocation size if that was deallocated as well.
    if (count_allocations_to_keep == 0)
    {
        m_metadata->first_allocation_size = 0;
    }

    // Recalculate the next offset at which we can allocate memory.
    m_metadata->next_allocation_offset = get_next_allocation_offset();

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("deallocate");
    }

    return error_code_t::success;
}

stack_allocator_metadata_t* stack_allocator_t::get_metadata() const
{
    return m_metadata;
}

size_t stack_allocator_t::get_allocation_count() const
{
    if (m_metadata == nullptr)
    {
        return 0;
    }

    return m_metadata->count_allocations;
}

stack_allocator_allocation_t* stack_allocator_t::get_allocation_record(size_t allocation_number) const
{
    if (m_metadata == nullptr)
    {
        return nullptr;
    }

    if (allocation_number == 0 || allocation_number > m_metadata->count_allocations)
    {
        return nullptr;
    }

    address_offset_t metadata_offset = get_offset(reinterpret_cast<uint8_t*>(m_metadata));
    address_offset_t allocation_record_offset = metadata_offset - allocation_number * sizeof(stack_allocator_allocation_t);
    uint8_t* allocation_record_address = get_address(allocation_record_offset);
    stack_allocator_allocation_t* allocation_record = reinterpret_cast<stack_allocator_allocation_t*>(allocation_record_address);
    return allocation_record;
}

address_offset_t stack_allocator_t::get_next_allocation_offset() const
{
    if (m_metadata == nullptr)
    {
        return 0;
    }

    size_t count_allocations = m_metadata->count_allocations;

    // Iterate over our allocation entries
    // and find out the offset at which we can next allocate memory.
    // We skip the first record because its size is stored differently.
    address_offset_t next_allocation_offset = m_base_memory_offset + m_metadata->first_allocation_size;
    for (size_t allocation_number = 2; allocation_number <= count_allocations; allocation_number++)
    {
        // Get allocation information object.
        stack_allocator_allocation_t* allocation_record = get_allocation_record(allocation_number);

        // Get actual allocation offset.
        address_offset_t allocation_offset = allocation_record->memory_offset;

        // Skip allocation records that indicate deletes.
        if (allocation_offset == 0)
        {
            continue;
        }

        // Get allocation prefix pointer, to get the allocation size.
        memory_allocation_metadata_t* allocation_metadata = read_allocation_metadata(allocation_offset);

        // Add allocation size.
        next_allocation_offset += allocation_metadata->allocation_size;
    }

    return next_allocation_offset;
}

void stack_allocator_t::output_debugging_information(const string& context_description) const
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "  Stack allocator information for context: " << context_description << ":" << endl;

    if (m_metadata == nullptr)
    {
        cout << "    Stack allocator has not been initialized." << endl;
    }

    cout << "    Count allocations = " << m_metadata->count_allocations << endl;
    cout << "    First allocation size = " << m_metadata->first_allocation_size << endl;
    cout << "    Next allocation offset = " << m_metadata->next_allocation_offset << endl;
    cout << c_debug_output_separator_line_end << endl;
}
