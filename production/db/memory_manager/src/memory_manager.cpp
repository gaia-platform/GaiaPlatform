/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "memory_manager.hpp"

#include <unistd.h>

#include <iostream>
#include <memory>

#include "gaia_internal/common/retail_assert.hpp"

#include "bitmap.hpp"
#include "chunk_manager.hpp"
#include "memory_types.hpp"

using namespace std;

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace memory_manager
{

inline void validate_metadata(memory_manager_metadata_t* metadata)
{
    ASSERT_PRECONDITION(metadata != nullptr, "Memory manager was not initialized!");
}

void memory_manager_t::initialize(
    uint8_t* memory_address,
    size_t memory_size)
{
    bool initialize_memory = true;
    initialize_internal(memory_address, memory_size, initialize_memory);
}

void memory_manager_t::load(
    uint8_t* memory_address,
    size_t memory_size)
{
    bool initialize_memory = false;
    initialize_internal(memory_address, memory_size, initialize_memory);
}

void memory_manager_t::initialize_internal(
    uint8_t* memory_address,
    size_t memory_size,
    bool initialize_memory)
{
    // Sanity checks.
    ASSERT_PRECONDITION(
        memory_address != nullptr,
        "memory_manager_t::initialize_internal() was called with a null memory address!");
    ASSERT_PRECONDITION(
        (reinterpret_cast<size_t>(memory_address)) % c_allocation_alignment == 0,
        "memory_manager_t::initialize_internal() was called with a misaligned memory address!");
    ASSERT_PRECONDITION(
        memory_size > 0,
        "memory_manager_t::initialize_internal() was called with a 0 memory size!");
    ASSERT_PRECONDITION(
        memory_size % c_chunk_size == 0,
        "memory_manager_t::initialize_internal() was called with a memory size that is not a multiple of chunk size (4MB)!");

    // Save our parameters.
    m_base_memory_address = memory_address;
    m_total_memory_size = memory_size;

    // Also initialize our offsets.
    m_start_memory_offset = 0;

    // Now that we set our parameters, we can do one last sanity check.
    validate_managed_memory_range();

    // Map the metadata information for quick reference.
    uint8_t* metadata_address = m_base_memory_address;
    m_metadata = reinterpret_cast<memory_manager_metadata_t*>(metadata_address);

    if (initialize_memory)
    {
        m_metadata->clear();
        m_metadata->start_unused_memory_offset = c_chunk_size;
    }

    if (m_execution_flags.enable_console_output)
    {
        cout
            << "  Configuration - enable_extra_validations = "
            << m_execution_flags.enable_extra_validations << endl;

        output_debugging_information(initialize_memory ? "initialize" : "load");
    }
}

address_offset_t memory_manager_t::allocate_chunk() const
{
    address_offset_t allocated_memory_offset = allocate_from_deallocated_memory();

    if (allocated_memory_offset == c_invalid_address_offset)
    {
        allocated_memory_offset = allocate_from_unused_memory();
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(__func__);
    }

    if (allocated_memory_offset != c_invalid_address_offset)
    {
        ASSERT_POSTCONDITION(
            allocated_memory_offset % c_chunk_size == 0,
            "Chunk allocations should be made on chunk size boundaries!");
    }

    return allocated_memory_offset;
}

void memory_manager_t::deallocate_chunk(address_offset_t chunk_address_offset) const
{
    auto chunk_offset = get_chunk_offset(chunk_address_offset);

    validate_metadata(m_metadata);
    ASSERT_PRECONDITION(
        chunk_offset >= c_first_chunk_offset && chunk_offset <= c_last_chunk_offset,
        "Chunk offset passed to deallocate_chunk() is out of bounds");

    while (!try_mark_chunk_used_status(chunk_offset, false))
    {
        // Retry until we succeed.
        // Only one thread (client session or compaction) should be calling this method,
        // so any failures are due to bitmap updates made for different chunks.
        ASSERT_INVARIANT(is_chunk_marked_as_used(chunk_offset), "Another thread has marked chunk as unused!");
    }

    // If the current chunk offset exceeds the highest deallocated chunk offset watermark,
    // then try updating the watermark until either we or another thread succeeds.
    chunk_offset_t highest_deallocated_chunk_offset = m_metadata->highest_deallocated_chunk_offset;
    while (chunk_offset > highest_deallocated_chunk_offset)
    {
        m_metadata->highest_deallocated_chunk_offset.compare_exchange_strong(
            highest_deallocated_chunk_offset, chunk_offset);
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(__func__);
    }
}

void memory_manager_t::deallocate(address_offset_t object_offset) const
{
    validate_metadata(m_metadata);
    validate_offset(object_offset);

    address_offset_t chunk_address_offset = get_chunk_address_offset(object_offset);
    slot_offset_t slot_offset = get_slot_offset(object_offset);

    chunk_manager_t chunk_manager;
    chunk_manager.load(m_base_memory_address, chunk_address_offset);

    while (!chunk_manager.try_mark_slot_used_status(slot_offset, false))
    {
        // Retry until we succeed.
        // An object should be deallocated by a single thread - the one
        // corresponding to the session that updated this copy of the object.
        ASSERT_INVARIANT(
            chunk_manager.is_slot_marked_as_used(slot_offset),
            "Another thread has already deallocated this object!");
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(__func__);
    }
}

address_offset_t memory_manager_t::allocate_from_deallocated_memory() const
{
    validate_metadata(m_metadata);

    // We want to prevent the search from finding unset bits for chunks that were never allocated,
    // so we limit the search up to the highest deallocated chunk offset known so far,
    // and then we need to subtract c_first_chunk_offset from that,
    // because bitmap indexes of a chunk offset are relative to c_first_chunk_offset.
    chunk_offset_t highest_deallocated_chunk_offset = m_metadata->highest_deallocated_chunk_offset;
    if (highest_deallocated_chunk_offset == c_invalid_chunk_offset)
    {
        return c_invalid_address_offset;
    }
    auto end_limit_bit_index = static_cast<size_t>(highest_deallocated_chunk_offset) - c_first_chunk_offset + 1;
    size_t first_unset_bit_index = c_max_bit_index;
    bool has_claimed_chunk = false;
    address_offset_t allocation_offset = c_invalid_address_offset;

    while ((first_unset_bit_index = find_first_unset_bit(
                m_metadata->chunk_bitmap,
                memory_manager_metadata_t::c_chunk_bitmap_size,
                end_limit_bit_index))
           != c_max_bit_index)
    {
        ASSERT_INVARIANT(first_unset_bit_index < end_limit_bit_index, "First unset bit index is outside the searched range!");

        auto chunk_offset = static_cast<chunk_offset_t>(first_unset_bit_index + c_first_chunk_offset);

        while ((has_claimed_chunk = try_mark_chunk_used_status(chunk_offset, true)) == false)
        {
            // If someone else claimed the chunk, look for another one;
            // otherwise, keep trying to claim the current one.
            if (is_chunk_marked_as_used(chunk_offset))
            {
                break;
            }
        }

        if (has_claimed_chunk)
        {
            allocation_offset = chunk_offset * c_chunk_size;
            break;
        }
    }

    if (m_execution_flags.enable_console_output
        && allocation_offset != c_invalid_address_offset)
    {
        cout << "\nAllocated chunk at offset " << allocation_offset << " from deallocated memory." << endl;
    }

    return allocation_offset;
}

size_t memory_manager_t::get_unused_memory_size() const
{
    validate_metadata(m_metadata);

    if (m_metadata->start_unused_memory_offset > m_total_memory_size)
    {
        return 0;
    }

    size_t available_size = m_total_memory_size - m_metadata->start_unused_memory_offset;

    return available_size;
}

address_offset_t memory_manager_t::allocate_from_unused_memory() const
{
    validate_metadata(m_metadata);

    // We allocate memory in chunk increments, so if any is left, it's large enough for a chunk.
    if (get_unused_memory_size() == 0)
    {
        return c_invalid_address_offset;
    }

    // Claim the space.
    // We use this approach instead of CAS to prevent the need for retrying
    // in case of concurrent updates.
    address_offset_t old_next_allocation_offset = m_metadata->start_unused_memory_offset.fetch_add(c_chunk_size);
    address_offset_t new_next_allocation_offset = old_next_allocation_offset + c_chunk_size;

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (new_next_allocation_offset > m_total_memory_size)
    {
        // We're going to leave the metadata offset indicating past the end of our memory block.
        // It doesn't matter because its use has ended with the exhaustion of unused memory.
        // From this point on, we can only perform allocations from deallocated chunks.
        ASSERT_INVARIANT(
            m_metadata->start_unused_memory_offset > m_total_memory_size,
            "Metadata offset should now point past the end of our memory range!");

        return c_invalid_address_offset;
    }

    // Our allocation has succeeded.
    address_offset_t allocation_offset = old_next_allocation_offset;
    chunk_offset_t chunk_offset = get_chunk_offset(allocation_offset);

    while (!try_mark_chunk_used_status(chunk_offset, true))
    {
        // Retry until we succeed.
        // We already claimed the chunk when we bumped start_unused_memory_offset,
        // so all failures must be due to concurrent updates of the bitmap word,
        // not because of a conflict on this particular chunk's bit.
        ASSERT_INVARIANT(!is_chunk_marked_as_used(chunk_offset), "Another thread has marked chunk as used!");
    }

    if (m_execution_flags.enable_console_output)
    {
        cout << "\nAllocated chunk at offset " << allocation_offset << " from unused memory." << endl;
    }

    return allocation_offset;
}

bool memory_manager_t::is_chunk_marked_as_used(chunk_offset_t chunk_offset) const
{
    validate_metadata(m_metadata);
    ASSERT_PRECONDITION(
        chunk_offset >= c_first_chunk_offset && chunk_offset <= c_last_chunk_offset,
        "Chunk offset passed to is_chunk_marked_as_used() is out of bounds");

    size_t bit_index = chunk_offset - c_first_chunk_offset;

    return is_bit_set(
        m_metadata->chunk_bitmap,
        memory_manager_metadata_t::c_chunk_bitmap_size,
        bit_index);
}

bool memory_manager_t::try_mark_chunk_used_status(chunk_offset_t chunk_offset, bool is_used) const
{
    validate_metadata(m_metadata);
    ASSERT_PRECONDITION(
        chunk_offset >= c_first_chunk_offset && chunk_offset <= c_last_chunk_offset,
        "Chunk offset passed to try_mark_chunk_used_status() is out of bounds");

    size_t bit_index = chunk_offset - c_first_chunk_offset;

    return try_set_bit_value(
        m_metadata->chunk_bitmap,
        memory_manager_metadata_t::c_chunk_bitmap_size,
        bit_index,
        is_used);
}

void memory_manager_t::output_debugging_information(const string& context_description) const
{
    cout << "\n"
         << c_debug_output_separator_line_start << endl;
    cout << "Debugging output for context: " << context_description << ":" << endl;
    cout << "  Start unused memory offset = " << m_metadata->start_unused_memory_offset << endl;
    cout << "  Highest deallocated chunk offset = " << m_metadata->highest_deallocated_chunk_offset << endl;
    cout << "  Unused memory size = " << get_unused_memory_size() << endl;
    cout << c_debug_output_separator_line_end << endl;
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
