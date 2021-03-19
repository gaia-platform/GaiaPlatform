/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "memory_manager.hpp"

#include <unistd.h>

#include <iostream>
#include <memory>

#include "gaia_internal/common/retail_assert.hpp"

#include "memory_types.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db::memory_manager;

memory_manager_t::memory_manager_t()
    : base_memory_manager_t()
{
    m_next_allocation_offset = c_invalid_address_offset;
}

void memory_manager_t::manage(
    uint8_t* memory_address,
    size_t memory_size)
{
    // Sanity checks.
    retail_assert(memory_address != nullptr, "memory_manager_t::manage() was called with a null memory address!");
    retail_assert(memory_size > 0, "memory_manager_t::manage() was called with a 0 memory size!");
    retail_assert(
        memory_size % c_chunk_size == 0,
        "memory_manager_t::manage() was called with a memory size that is not a multiple of chunk size (4MB)!");
    validate_address_alignment(memory_address);
    validate_size_alignment(memory_size);

    // Save our parameters.
    m_base_memory_address = memory_address;
    m_total_memory_size = memory_size;

    // Also initialize our offsets.
    m_start_memory_offset = 0;
    m_next_allocation_offset = 0;

    // Now that we set our parameters, we can do one last sanity check.
    validate_managed_memory_range();

    if (m_execution_flags.enable_console_output)
    {
        cout
            << "  Configuration - enable_extra_validations = "
            << m_execution_flags.enable_extra_validations << endl;

        output_debugging_information("manage");
    }
}

address_offset_t memory_manager_t::allocate_chunk()
{
    address_offset_t allocated_memory_offset = allocate_internal(c_chunk_size);

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("allocate_chunk");
    }

    if (allocated_memory_offset != c_invalid_address_offset)
    {
        // This check supersedes the validate_offset_alignment() check.
        retail_assert(
            allocated_memory_offset % c_chunk_size == 0,
            "Chunk allocations should be made on chunk size boundaries!");
    }

    return allocated_memory_offset;
}

address_offset_t memory_manager_t::allocate(size_t size)
{
    address_offset_t allocated_memory_offset = allocate_internal(size);

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("allocate");
    }

    if (allocated_memory_offset != c_invalid_address_offset)
    {
        validate_offset_alignment(allocated_memory_offset);
    }

    return allocated_memory_offset;
}

void memory_manager_t::deallocate(address_offset_t)
{
    // TODO: Implement memory deallocation (mark slots in corresponding chunk metadata as unused).
}

size_t memory_manager_t::get_available_memory_size() const
{
    size_t available_size = m_total_memory_size - m_next_allocation_offset;

    return available_size;
}

address_offset_t memory_manager_t::allocate_internal(size_t size)
{
    size = base_memory_manager_t::calculate_allocation_size(size);

    // If the allocation exhausts our memory, we cannot perform it.
    if (get_available_memory_size() < size)
    {
        return c_invalid_address_offset;
    }

    // Claim the space.
    address_offset_t old_next_allocation_offset = __sync_fetch_and_add(
        &m_next_allocation_offset,
        size);
    address_offset_t new_next_allocation_offset = old_next_allocation_offset + size;

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (new_next_allocation_offset > m_total_memory_size)
    {
        // We exhausted the memory so we must undo our update.
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

        return c_invalid_address_offset;
    }

    // Our allocation has succeeded.
    address_offset_t allocation_offset = old_next_allocation_offset;

    if (m_execution_flags.enable_console_output)
    {
        cout << "\nAllocated " << size << " bytes at offset " << allocation_offset << " from main memory." << endl;
    }

    return allocation_offset;
}

void memory_manager_t::output_debugging_information(const string& context_description) const
{
    cout << "\n"
         << c_debug_output_separator_line_start << endl;
    cout << "Debugging output for context: " << context_description << ":" << endl;
    cout << "  Next allocation offset = " << m_next_allocation_offset << endl;
    cout << "  Available memory = " << get_available_memory_size() << endl;
    cout << c_debug_output_separator_line_end << endl;
}
