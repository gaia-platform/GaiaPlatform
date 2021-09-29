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
#include "db_helpers.hpp"
#include "db_internal_types.hpp"
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
        memory_size % c_chunk_size_bytes == 0,
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
    }

    if (m_execution_flags.enable_console_output)
    {
        cout
            << "  Configuration - enable_extra_validations = "
            << m_execution_flags.enable_extra_validations << endl;

        output_debugging_information(initialize_memory ? "initialize" : "load");
    }
}

chunk_offset_t memory_manager_t::allocate_unused_chunk()
{
    // We claim the next available unused chunk, and keep trying until we succeed.
    // (This is not wait-free, but conflicts should be rare.)
    while (true)
    {
        // Get the next available unused chunk offset.
        chunk_offset_t next_chunk_offset = m_metadata->next_available_unused_chunk_offset++;

        // If we've run out of memory, return the invalid offset.
        if (next_chunk_offset > c_last_chunk_offset)
        {
            return c_invalid_chunk_offset;
        }

        // Now try to claim this chunk.
        chunk_manager_t chunk_manager;
        chunk_manager.load(next_chunk_offset);
        if (chunk_manager.allocate_chunk())
        {
            return next_chunk_offset;
        }
    }
}

chunk_offset_t memory_manager_t::allocate_reused_chunk()
{
    // Starting from the first chunk, scan for the first available reused chunk,
    // up to a snapshot of next_available_unused_chunk_offset. If we fail to
    // claim an available chunk, restart the scan. (If we fail to find or claim
    // any reused chunks, then the caller can allocate a new chunk from unused
    // memory.) Since next_available_unused_chunk_offset can be concurrently
    // advanced, and chunks can also be deallocated behind our scan pointer,
    // this search is best-effort; we could miss a chunk deallocated
    // concurrently with our scan.
    size_t first_unused_chunk_offset = m_metadata->next_available_unused_chunk_offset;
    if (first_unused_chunk_offset != c_first_chunk_offset)
    {
        while (true)
        {
            size_t found_index = find_first_unset_bit(
                m_metadata->allocated_chunks_bitmap,
                memory_manager_metadata_t::c_chunk_bitmap_words_size,
                first_unused_chunk_offset);

            if (found_index == c_max_bit_index)
            {
                break;
            }

            // We found an available chunk, so try to claim it.
            auto available_chunk_offset = static_cast<chunk_offset_t>(found_index);
            chunk_manager_t chunk_manager;
            chunk_manager.load(available_chunk_offset);
            if (chunk_manager.allocate_chunk())
            {
                return available_chunk_offset;
            }
        }
    }

    // We either couldn't find any reused chunks, or were unable to claim any of
    // those we found.
    return c_invalid_chunk_offset;
}

// Allocates the first available chunk.
chunk_offset_t memory_manager_t::allocate_chunk()
{
    // First try to reuse a deallocated chunk.
    chunk_offset_t allocated_chunk_offset = allocate_reused_chunk();
    if (allocated_chunk_offset != c_invalid_chunk_offset)
    {
#ifdef DEBUG
        // In debug mode, we write-protect all allocations after writes are
        // complete. (We do this by allocating only on page boundaries and
        // write-protecting the pages used for allocations.) If we do not remove
        // this write protection from the deallocated chunk's pages, then when the
        // chunk is reused, any writes will cause a SIGSEGV signal to be sent to the
        // writing process.
        gaia_offset_t first_data_page_offset = offset_from_chunk_and_slot(allocated_chunk_offset, c_first_slot_offset);
        void* data_pages_initial_address = page_address_from_offset(first_data_page_offset);

        if (-1 == ::mprotect(data_pages_initial_address, c_data_pages_size_bytes, PROT_READ | PROT_WRITE))
        {
            throw_system_error("mprotect(PROT_READ|PROT_WRITE) failed!");
        }
#endif
    }

    // If no deallocated chunk is available, then claim the next chunk from unused memory.
    if (allocated_chunk_offset == c_invalid_chunk_offset)
    {
        allocated_chunk_offset = allocate_unused_chunk();
    }

    // At this point, we must either have a valid chunk offset, or we have run out of memory.
    ASSERT_INVARIANT(
        (allocated_chunk_offset != c_invalid_chunk_offset) || (m_metadata->next_available_unused_chunk_offset > c_last_chunk_offset),
        "Chunk allocation cannot fail unless memory is exhausted!");

    return allocated_chunk_offset;
}

void memory_manager_t::update_chunk_allocation_status(chunk_offset_t chunk_offset, bool is_allocated)
{
    safe_set_bit_value(
        m_metadata->allocated_chunks_bitmap,
        memory_manager_metadata_t::c_chunk_bitmap_words_size,
        chunk_offset, is_allocated);
}

size_t memory_manager_t::get_unused_memory_size() const
{
    validate_metadata(m_metadata);

    if (m_metadata->next_available_unused_chunk_offset > (m_total_memory_size * c_chunk_size_bytes))
    {
        return 0;
    }

    size_t available_size = m_total_memory_size - (m_metadata->next_available_unused_chunk_offset * c_chunk_size_bytes);

    return available_size;
}

void memory_manager_t::output_debugging_information(const string& context_description) const
{
    cout << "\n"
         << c_debug_output_separator_line_start << endl;
    cout << "Debugging output for context: " << context_description << ":" << endl;
    cout << "  Start unused memory offset = " << (m_metadata->next_available_unused_chunk_offset * c_chunk_size_bytes) << endl;
    cout << "  Unused memory size = " << get_unused_memory_size() << endl;
    cout << c_debug_output_separator_line_end << endl;
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
