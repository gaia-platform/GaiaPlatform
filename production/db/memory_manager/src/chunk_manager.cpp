/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "chunk_manager.hpp"

#include <iostream>

#include "gaia_internal/common/retail_assert.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db::memory_manager;

void chunk_manager_t::initialize(
    uint8_t* base_memory_address,
    address_offset_t memory_offset)
{
    bool initialize_memory = true;
    initialize_internal(base_memory_address, memory_offset, initialize_memory);
}

void chunk_manager_t::load(
    uint8_t* base_memory_address,
    address_offset_t memory_offset)
{
    bool initialize_memory = false;
    initialize_internal(base_memory_address, memory_offset, initialize_memory);
}

void chunk_manager_t::initialize_internal(
    uint8_t* base_memory_address,
    address_offset_t memory_offset,
    bool initialize_memory)
{
    retail_assert(
        base_memory_address != nullptr,
        "chunk_manager_t::initialize_internal was called with a null memory address!");
    retail_assert(
        memory_offset != c_invalid_address_offset,
        "chunk_manager_t::initialize_internal was called with an invalid memory offset!");
    retail_assert(
        memory_offset % c_chunk_size == 0,
        "chunk_manager_t::initialize_internal was called with a memory offset that is not a multiple of the chunk size (4MB)!");

    validate_address_alignment(base_memory_address);
    validate_offset_alignment(memory_offset);

    // Save our parameters.
    m_base_memory_address = base_memory_address;
    m_start_memory_offset = memory_offset;
    m_total_memory_size = c_chunk_size;

    // Now that we set our parameters, we can do one last sanity check.
    validate_managed_memory_range();

    // Map the metadata information for quick reference.
    uint8_t* metadata_address
        = m_base_memory_address + m_start_memory_offset;
    m_metadata = reinterpret_cast<chunk_manager_metadata_t*>(metadata_address);

    if (initialize_memory)
    {
        m_metadata->clear();
        m_metadata->last_committed_slot_offset = sizeof(chunk_manager_metadata_t) / c_allocation_slot_size;
        m_metadata->last_allocated_slot_offset = m_metadata->last_committed_slot_offset;
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(initialize_memory ? "initialize" : "load");
    }
}

address_offset_t chunk_manager_t::allocate(
    size_t memory_size) const
{
    retail_assert(m_metadata != nullptr, "Chunk manager was not initialized!");
    retail_assert(memory_size > 0, "Chunk manager requested allocation size is 0!");

    // Adjust the requested memory size, to ensure proper alignment.
    memory_size = calculate_allocation_size(memory_size);
    validate_size(memory_size);

    // Quick exit for memory requests that are way too large.
    if (memory_size > m_total_memory_size - m_metadata->last_allocated_slot_offset * c_allocation_slot_size)
    {
        return c_invalid_address_offset;
    }

    // Get number of slots required for this allocation and update last_allocated_slot_offset.
    slot_offset_t slot_count = memory_size / c_allocation_slot_size;
    slot_offset_t allocation_slot_offset = m_metadata->last_allocated_slot_offset;
    m_metadata->last_allocated_slot_offset += slot_count;

    // Convert the allocation_slot_offset into an allocation_address_offset.
    address_offset_t allocation_address_offset
        = m_start_memory_offset + (allocation_slot_offset * c_allocation_slot_size);

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("allocate");
    }

    return allocation_address_offset;
}

void chunk_manager_t::commit() const
{
    retail_assert(
        m_metadata->last_allocated_slot_offset >= m_metadata->last_committed_slot_offset,
        "last_allocated_slot_offset is lesser than last_committed_slot_offset!");

    m_metadata->last_committed_slot_offset = m_metadata->last_allocated_slot_offset;

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("commit");
    }
}

void chunk_manager_t::rollback() const
{
    retail_assert(
        m_metadata->last_allocated_slot_offset >= m_metadata->last_committed_slot_offset,
        "last_allocated_slot_offset is lesser than last_committed_slot_offset!");

    m_metadata->last_allocated_slot_offset = m_metadata->last_committed_slot_offset;

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("rollback");
    }
}

void chunk_manager_t::output_debugging_information(const string& context_description) const
{
    cout << "\n"
         << c_debug_output_separator_line_start << endl;
    cout << "  Chunk Manager information for context: " << context_description << ":" << endl;

    if (m_metadata == nullptr)
    {
        cout << "    Chunk Manager has not been initialized." << endl;
    }
    else
    {
        cout << "    Last committed slot offset = " << m_metadata->last_committed_slot_offset << endl;
        cout << "    Last allocated slot offset = " << m_metadata->last_allocated_slot_offset << endl;
    }

    cout << c_debug_output_separator_line_end << endl;
}
