/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "stack_allocator.hpp"

#include <iostream>

#include "gaia_internal/common/retail_assert.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db::memory_manager;

stack_allocator_t::stack_allocator_t()
{
    m_metadata = nullptr;
    m_has_been_freed = false;
}

void stack_allocator_t::initialize(
    uint8_t* base_memory_address,
    address_offset_t memory_offset,
    size_t memory_size)
{
    bool initialize_memory = true;
    initialize_internal(base_memory_address, memory_offset, memory_size, initialize_memory);
}

void stack_allocator_t::load(
    uint8_t* base_memory_address,
    address_offset_t memory_offset,
    size_t memory_size)
{
    bool initialize_memory = false;
    initialize_internal(base_memory_address, memory_offset, memory_size, initialize_memory);
}

void stack_allocator_t::initialize_internal(
    uint8_t* base_memory_address,
    address_offset_t memory_offset,
    size_t memory_size,
    bool initialize_memory)
{
    retail_assert(
        base_memory_address != nullptr,
        "stack_allocator_t::initialize_internal was called with a null memory address!");
    retail_assert(
        memory_offset != c_invalid_offset,
        "stack_allocator_t::initialize_internal was called with an invalid memory offset!");
    retail_assert(
        memory_size > 0,
        "stack_allocator_t::initialize_internal was called with a 0 memory size!");

    validate_address_alignment(base_memory_address);
    validate_offset_alignment(memory_offset);
    validate_size_alignment(memory_size);

    retail_assert(
        memory_size > sizeof(stack_allocator_metadata_t),
        "Stack allocator was initialized with too little memory!");

    // Save our parameters.
    m_base_memory_address = base_memory_address;
    m_start_memory_offset = memory_offset;
    m_total_memory_size = memory_size;

    // Now that we set our parameters, we can do one last sanity check.
    validate_managed_memory_range();

    // Map the metadata information for quick reference.
    uint8_t* metadata_address
        = m_base_memory_address + m_start_memory_offset + m_total_memory_size - sizeof(stack_allocator_metadata_t);
    m_metadata = reinterpret_cast<stack_allocator_metadata_t*>(metadata_address);

    if (initialize_memory)
    {
        m_metadata->clear();
        m_metadata->next_allocation_offset = m_start_memory_offset;
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(initialize_memory ? "initialize" : "load");
    }
}

address_offset_t stack_allocator_t::allocate(
    slot_id_t slot_id,
    address_offset_t old_slot_offset,
    size_t memory_size) const
{
    retail_assert(m_metadata != nullptr, "Stack allocator was not initialized!");

    if (old_slot_offset != c_invalid_offset)
    {
        validate_offset_alignment(old_slot_offset);
    }

    if (memory_size == 0)
    {
        retail_assert(
            old_slot_offset != c_invalid_offset,
            "A deallocation call was made without providing the address offset to deallocate!");
    }
    else
    {
        // Adjust the requested memory size, to ensure proper alignment.
        memory_size = calculate_allocation_size(memory_size);
        validate_size(memory_size);
    }

    // Quick exit for memory requests that are way too large.
    if (memory_size > m_total_memory_size)
    {
        return c_invalid_offset;
    }

    // Each allocation will get its own metadata.
    size_t size_to_allocate = (memory_size == 0) ? 0 : memory_size + sizeof(memory_allocation_metadata_t);
    size_t count_allocations = m_metadata->count_allocations;

    address_offset_t next_allocation_offset = m_metadata->next_allocation_offset;
    address_offset_t metadata_offset = get_offset(reinterpret_cast<uint8_t*>(m_metadata));

    // Now we can do a more accurate check about whether this allocation fits in our memory.
    // The "+1" accounts for our current planned allocation.
    if (next_allocation_offset + size_to_allocate
        > metadata_offset - (count_allocations + 1) * sizeof(stack_allocator_allocation_t))
    {
        return c_invalid_offset;
    }

    if (size_to_allocate > 0)
    {
        // Perform allocation.
        uint8_t* next_allocation_address = get_address(next_allocation_offset);
        auto next_allocation_metadata = reinterpret_cast<memory_allocation_metadata_t*>(next_allocation_address);
        next_allocation_metadata->allocation_size = size_to_allocate;
    }

    m_metadata->count_allocations++;

    // Record allocation information.
    stack_allocator_allocation_t* allocation_record = get_allocation_record(m_metadata->count_allocations);
    allocation_record->clear();

    allocation_record->slot_id = slot_id;
    allocation_record->old_memory_offset = old_slot_offset;

    // Set allocation offset.
    address_offset_t allocated_memory_offset = c_invalid_offset;
    if (size_to_allocate > 0)
    {
        allocation_record->memory_offset = next_allocation_offset + sizeof(memory_allocation_metadata_t);
        allocated_memory_offset = allocation_record->memory_offset;

        // Verify proper allocation alignment.
        retail_assert(
            allocated_memory_offset % c_allocation_alignment == 0,
            "Stack allocator memory allocation was not made on a 64B boundary!");

        m_metadata->next_allocation_offset += size_to_allocate;
    }

    if (m_execution_flags.enable_console_output)
    {
        string context = (size_to_allocate > 0) ? "allocate" : "deallocate";
        output_debugging_information(context);
    }

    return allocated_memory_offset;
}

void stack_allocator_t::deallocate(slot_id_t slot_id, address_offset_t slot_offset) const
{
    address_offset_t allocated_offset = allocate(slot_id, slot_offset, 0);

    retail_assert(
        allocated_offset == c_invalid_offset,
        "allocate(0) should have returned a c_invalid_offset offset!");
}

void stack_allocator_t::deallocate(size_t count_allocations_to_keep) const
{
    retail_assert(m_metadata != nullptr, "Stack allocator was not initialized!");

    retail_assert(
        count_allocations_to_keep <= m_metadata->count_allocations,
        "stack_allocator_t::deallocate() was called with more allocations than were made!");

    m_metadata->count_allocations = count_allocations_to_keep;

    // Recalculate the next offset at which we can allocate memory.
    m_metadata->next_allocation_offset = calculate_next_allocation_offset();

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("deallocate");
    }
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
    address_offset_t allocation_record_offset
        = metadata_offset - allocation_number * sizeof(stack_allocator_allocation_t);
    uint8_t* allocation_record_address = get_address(allocation_record_offset);
    auto allocation_record = reinterpret_cast<stack_allocator_allocation_t*>(allocation_record_address);
    return allocation_record;
}

address_offset_t stack_allocator_t::calculate_next_allocation_offset() const
{
    if (m_metadata == nullptr)
    {
        return c_invalid_offset;
    }

    size_t count_allocations = m_metadata->count_allocations;

    // Default to starting to allocate from beginning.
    // This will get overwritten if we find any allocation in the list of our allocation entries
    // (which may consist only of deallocations).
    address_offset_t next_allocation_offset = m_start_memory_offset;

    // Iterate backwards over our allocation entries
    // and find out the offset at which we can next allocate memory
    // using the last allocation record information.
    for (size_t allocation_number = count_allocations; allocation_number >= 1; --allocation_number)
    {
        // Get the allocation information object.
        stack_allocator_allocation_t* allocation_record = get_allocation_record(allocation_number);

        // Get the actual allocation offset.
        address_offset_t allocation_offset = allocation_record->memory_offset;

        // Skip the allocation records that just indicate deallocations.
        if (allocation_offset == c_invalid_offset)
        {
            continue;
        }

        // Get the allocation metadata pointer, to get the allocation size.
        memory_allocation_metadata_t* allocation_metadata = read_allocation_metadata(allocation_offset);

        // The next allocation offset should follow this allocation,
        // so add its size to its offset and adjust for the size of the allocation metadata.
        next_allocation_offset = allocation_offset + allocation_metadata->allocation_size
            - sizeof(memory_allocation_metadata_t);

        // We're done - we've found the last actual allocation record.
        break;
    }

    return next_allocation_offset;
}

void stack_allocator_t::mark_as_freed()
{
    m_has_been_freed = true;
}

bool stack_allocator_t::has_been_freed()
{
    return m_has_been_freed;
}

void stack_allocator_t::output_debugging_information(const string& context_description) const
{
    cout << endl
         << c_debug_output_separator_line_start << endl;
    cout << "  Stack allocator information for context: " << context_description << ":" << endl;

    if (m_metadata == nullptr)
    {
        cout << "    Stack allocator has not been initialized." << endl;
    }
    else
    {
        cout << "    Count allocations = " << m_metadata->count_allocations << endl;
        cout << "    Next allocation offset = " << m_metadata->next_allocation_offset << endl;
    }

    cout << c_debug_output_separator_line_end << endl;
}
