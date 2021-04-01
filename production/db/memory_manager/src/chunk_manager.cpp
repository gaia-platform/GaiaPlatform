/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "chunk_manager.hpp"

#include <iostream>

#include "gaia_internal/common/retail_assert.hpp"

#include "bitmap.hpp"

using namespace std;

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace memory_manager
{

inline void validate_metadata(chunk_manager_metadata_t* metadata)
{
    retail_assert(metadata != nullptr, "Chunk manager was not initialized!");
}

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
        "chunk_manager_t::initialize_internal() was called with a null memory address!");
    retail_assert(
        memory_offset != c_invalid_address_offset,
        "chunk_manager_t::initialize_internal() was called with an invalid memory offset!");
    retail_assert(
        memory_offset % c_chunk_size == 0,
        "chunk_manager_t::initialize_internal() was called with a memory offset that is not a multiple of the chunk size (4MB)!");

    validate_address_alignment(base_memory_address);
    validate_offset_alignment(memory_offset);

    // Save our parameters.
    m_base_memory_address = base_memory_address;
    m_start_memory_offset = memory_offset;
    m_total_memory_size = c_chunk_size;

    // Now that we set our parameters, we can do one last sanity check.
    validate_managed_memory_range();

    // Map the metadata information for quick reference.
    uint8_t* metadata_address = m_base_memory_address + m_start_memory_offset;
    m_metadata = reinterpret_cast<chunk_manager_metadata_t*>(metadata_address);

    if (initialize_memory)
    {
        m_metadata->clear();
        m_metadata->last_committed_slot_offset = sizeof(chunk_manager_metadata_t) / c_slot_size - 1;
    }

    m_last_allocated_slot_offset = m_metadata->last_committed_slot_offset;

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(initialize_memory ? "initialize" : "load");
    }
}

address_offset_t chunk_manager_t::allocate(
    size_t memory_size)
{
    validate_metadata(m_metadata);

    // Adjust the requested memory size, to ensure proper alignment.
    memory_size = calculate_allocation_size(memory_size);
    validate_size(memory_size);

    // Quick exit for memory requests that are way too large.
    size_t available_memory_size = (m_last_allocated_slot_offset == c_last_slot_offset)
        ? 0
        : m_total_memory_size - (m_last_allocated_slot_offset + 1) * c_slot_size;
    if (memory_size > available_memory_size)
    {
        return c_invalid_address_offset;
    }

    // Get the number of slots required for this allocation and update m_last_allocated_slot_offset.
    slot_offset_t slot_count = memory_size / c_slot_size;
    retail_assert(slot_count >= 1, "An allocation should use at least one slot!");
    slot_offset_t allocation_slot_offset = m_last_allocated_slot_offset + 1;
    retail_assert(
        m_last_allocated_slot_offset + slot_count > m_last_allocated_slot_offset,
        "The update of m_last_allocated_slot_offset would cause an integer overflow!");

    // Mark the first allocation slot as used in the metadata slot bitmap.
    // This marking is done immediately, before we get a decision for our transaction.
    // If our process dies, the server will ignore this portion of the bitmap
    // based on the value of the metadata's last_committed_slot_offset.
    while (!try_mark_slot_use(allocation_slot_offset, true))
    {
        // Failure can be due to another thread attempting to do GC.
        // Allocations should only be performed on one thread,
        // so we just need to retry until we succeed.
        retail_assert(!is_slot_marked_as_used(allocation_slot_offset), "Another thread has marked slot as used!");
    };

    m_last_allocated_slot_offset += slot_count;

    // Convert the allocation_slot_offset into an allocation_address_offset.
    address_offset_t allocation_address_offset
        = m_start_memory_offset + (allocation_slot_offset * c_slot_size);

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(__func__);
    }

    return allocation_address_offset;
}

void chunk_manager_t::commit(slot_offset_t last_allocated_offset)
{
    validate_metadata(m_metadata);
    retail_assert(
        m_last_allocated_slot_offset >= m_metadata->last_committed_slot_offset,
        "m_last_allocated_slot_offset is lesser than last_committed_slot_offset!");

    if (last_allocated_offset != c_invalid_slot_offset)
    {
        retail_assert(
            m_last_allocated_slot_offset == m_metadata->last_committed_slot_offset,
            "commit() should only be called with a valid offset if m_last_allocated_slot_offset has not been changed!");

        m_last_allocated_slot_offset = last_allocated_offset;
    }

    m_metadata->last_committed_slot_offset = m_last_allocated_slot_offset;

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(__func__);
    }
}

void chunk_manager_t::rollback()
{
    validate_metadata(m_metadata);
    retail_assert(
        m_last_allocated_slot_offset >= m_metadata->last_committed_slot_offset,
        "m_last_allocated_slot_offset is lesser than last_committed_slot_offset!");

    slot_offset_t first_uncommitted_allocation_slot_offset = m_metadata->last_committed_slot_offset + 1;
    size_t slot_count = m_last_allocated_slot_offset - m_metadata->last_committed_slot_offset;

    // There may be no work to do if no allocations were actually made.
    if (slot_count)
    {
        // We don't know how many allocations were actually made,
        // so we'll just reset all the bits for the uncommitted slots.
        mark_slot_range_use(first_uncommitted_allocation_slot_offset, slot_count, false);

        m_last_allocated_slot_offset = m_metadata->last_committed_slot_offset;
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(__func__);
    }
}

bool chunk_manager_t::is_slot_marked_as_used(slot_offset_t slot_offset) const
{
    validate_metadata(m_metadata);
    retail_assert(
        slot_offset >= c_first_slot_offset && slot_offset <= c_last_slot_offset,
        "Slot offset passed to is_slot_marked_as_used() is out of bounds");

    uint64_t bit_index = slot_offset - c_first_slot_offset;

    return is_bit_set(
        m_metadata->slot_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_size,
        bit_index);
}

bool chunk_manager_t::try_mark_slot_use(slot_offset_t slot_offset, bool used) const
{
    validate_metadata(m_metadata);
    retail_assert(
        slot_offset >= c_first_slot_offset && slot_offset <= c_last_slot_offset,
        "Slot offset passed to try_mark_slot_use() is out of bounds");

    uint64_t bit_index = slot_offset - c_first_slot_offset;

    return try_set_bit_value(
        m_metadata->slot_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_size,
        bit_index,
        used);
}

void chunk_manager_t::mark_slot_range_use(slot_offset_t start_slot_offset, slot_offset_t slot_count, bool used) const
{
    validate_metadata(m_metadata);
    retail_assert(
        start_slot_offset >= c_first_slot_offset && start_slot_offset <= c_last_slot_offset,
        "Slot offset passed to mark_slot_range_use() is out of bounds");

    uint64_t start_bit_index = start_slot_offset - c_first_slot_offset;

    set_bit_range_value(
        m_metadata->slot_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_size,
        start_bit_index,
        slot_count,
        used);
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
        cout << "    Last allocated slot offset = " << m_last_allocated_slot_offset << endl;
    }

    cout << c_debug_output_separator_line_end << endl;
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
