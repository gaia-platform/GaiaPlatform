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
    retail_assert(metadata != nullptr, "Memory manager was not initialized!");
}

void memory_manager_t::initialize(
    uint8_t* memory_address,
    size_t memory_size)
{
    bool initialize_memory = true;
    initialize_internal(memory_address, memory_size, initialize_memory, __func__);
}

void memory_manager_t::load(
    uint8_t* memory_address,
    size_t memory_size)
{
    bool initialize_memory = false;
    initialize_internal(memory_address, memory_size, initialize_memory, __func__);
}

void memory_manager_t::initialize_internal(
    uint8_t* memory_address,
    size_t memory_size,
    bool initialize_memory,
    const string& caller_name)
{
    // Sanity checks.
    retail_assert(
        memory_address != nullptr,
        string("memory_manager_t::") + caller_name + "() was called with a null memory address!");
    retail_assert(
        memory_size > 0,
        string("memory_manager_t::") + caller_name + "() was called with a 0 memory size!");
    retail_assert(
        memory_size % c_chunk_size == 0,
        string("memory_manager_t::") + caller_name + "() was called with a memory size that is not a multiple of chunk size (4MB)!");
    validate_address_alignment(memory_address);
    validate_size_alignment(memory_size);

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

        output_debugging_information(caller_name);
    }
}

address_offset_t memory_manager_t::allocate_chunk() const
{
    address_offset_t allocated_memory_offset = allocate_from_freed_memory();

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
        // This check supersedes the validate_offset_alignment() check.
        retail_assert(
            allocated_memory_offset % c_chunk_size == 0,
            "Chunk allocations should be made on chunk size boundaries!");
    }

    return allocated_memory_offset;
}

address_offset_t memory_manager_t::allocate(size_t size) const
{
    address_offset_t allocated_memory_offset = allocate_from_unused_memory(size);

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(__func__);
    }

    if (allocated_memory_offset != c_invalid_address_offset)
    {
        validate_offset_alignment(allocated_memory_offset);
    }

    return allocated_memory_offset;
}

void memory_manager_t::deallocate_chunk(chunk_offset_t chunk_offset) const
{
    validate_metadata(m_metadata);
    retail_assert(
        chunk_offset >= c_first_chunk_offset && chunk_offset <= c_last_chunk_offset,
        string("Chunk offset passed to  ") + __func__ + "() is out of bounds");

    while (!try_mark_chunk_use(chunk_offset, false))
    {
        // Keep trying!
        // Only the compaction thread should be calling this method,
        // so any failures are due to bitmap updates made for different chunks.
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(__func__);
    }
}

void memory_manager_t::deallocate(address_offset_t object_offset) const
{
    // TODO: Uncomment this code once client/server code is updated to use chunk managers.
    //
    // validate_metadata(m_metadata);
    // validate_offset(object_offset);

    // address_offset_t offset_within_chunk = object_offset % c_chunk_size;
    // address_offset_t chunk_address_offset = object_offset - offset_within_chunk;
    // auto slot_offset = static_cast<slot_offset_t>(offset_within_chunk / c_slot_size);

    // chunk_manager_t chunk_manager;
    // chunk_manager.load(m_base_memory_address, chunk_address_offset);

    // while (!chunk_manager.try_mark_slot_use(slot_offset, false))
    // {
    //     if (!chunk_manager.is_slot_marked_as_used(slot_offset))
    //     {
    //         // Someone else has already marked the slot as not being used,
    //         // so we can quit trying to do it ourselveds.
    //         break;
    //     }
    // }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information(__func__);
    }
}

size_t memory_manager_t::get_unused_memory_size() const
{
    validate_metadata(m_metadata);

    size_t available_size = m_total_memory_size - m_metadata->start_unused_memory_offset;

    return available_size;
}

address_offset_t memory_manager_t::allocate_from_freed_memory() const
{
    validate_metadata(m_metadata);

    uint64_t end_limit_bit_index = (m_metadata->start_unused_memory_offset / c_chunk_size) - 1;

    uint64_t first_unset_bit_index = c_max_bit_index;
    bool has_claimed_chunk = false;
    uint64_t allocation_offset = c_invalid_address_offset;

    while ((first_unset_bit_index = find_first_unset_bit(
                m_metadata->chunk_bitmap,
                memory_manager_metadata_t::c_chunk_bitmap_size,
                end_limit_bit_index))
           != c_max_bit_index)
    {
        auto chunk_offset = static_cast<chunk_offset_t>(first_unset_bit_index + c_first_chunk_offset);

        while ((has_claimed_chunk = try_mark_chunk_use(chunk_offset, true)) == false)
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
        cout << "\nAllocated chunk at offset " << allocation_offset << " from freed memory." << endl;
    }

    return allocation_offset;
}

address_offset_t memory_manager_t::allocate_from_unused_memory(size_t size) const
{
    validate_metadata(m_metadata);

    size = base_memory_manager_t::calculate_allocation_size(size);

    // If the allocation exhausts our memory, we cannot perform it.
    if (get_unused_memory_size() < size)
    {
        return c_invalid_address_offset;
    }

    // Claim the space.
    address_offset_t old_next_allocation_offset = __sync_fetch_and_add(
        &(m_metadata->start_unused_memory_offset),
        size);
    address_offset_t new_next_allocation_offset = old_next_allocation_offset + size;

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (new_next_allocation_offset > m_total_memory_size)
    {
        // We exhausted the memory so we must undo our update.
        while (!__sync_bool_compare_and_swap(
            &(m_metadata->start_unused_memory_offset),
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
    chunk_offset_t chunk_offset = allocation_offset / c_chunk_size;

    while (!try_mark_chunk_use(chunk_offset, true))
    {
        // Just retry until we succeed.
        // We already claimed the chunk, so all failures must be due to concurrent updates of the bitmap word,
        // not because of a conflict on this particular chunk's bit.
    }

    if (m_execution_flags.enable_console_output)
    {
        cout << "\nAllocated " << size << " bytes at offset " << allocation_offset << " from unused memory." << endl;
    }

    return allocation_offset;
}

bool memory_manager_t::is_chunk_marked_as_used(chunk_offset_t chunk_offset) const
{
    validate_metadata(m_metadata);
    retail_assert(
        chunk_offset >= c_first_chunk_offset && chunk_offset <= c_last_chunk_offset,
        string("Chunk offset passed to  ") + __func__ + "() is out of bounds");

    uint64_t bit_index = chunk_offset - c_first_chunk_offset;

    return is_bit_set(
        m_metadata->chunk_bitmap,
        memory_manager_metadata_t::c_chunk_bitmap_size,
        bit_index);
}

bool memory_manager_t::try_mark_chunk_use(chunk_offset_t chunk_offset, bool used) const
{
    validate_metadata(m_metadata);
    retail_assert(
        chunk_offset >= c_first_chunk_offset && chunk_offset <= c_last_chunk_offset,
        string("Chunk offset passed to ") + __func__ + "() is out of bounds");

    uint64_t bit_index = chunk_offset - c_first_chunk_offset;

    return try_set_bit_value(
        m_metadata->chunk_bitmap,
        memory_manager_metadata_t::c_chunk_bitmap_size,
        bit_index,
        used);
}

void memory_manager_t::output_debugging_information(const string& context_description) const
{
    cout << "\n"
         << c_debug_output_separator_line_start << endl;
    cout << "Debugging output for context: " << context_description << ":" << endl;
    cout << "  Start unused memory offset = " << m_metadata->start_unused_memory_offset << endl;
    cout << "  Unused memory size = " << get_unused_memory_size() << endl;
    cout << c_debug_output_separator_line_end << endl;
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
