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
        m_metadata->next_available_unused_chunk_offset = c_first_chunk_offset;
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
        if (m_metadata->apply_chunk_transition(
                next_chunk_offset, chunk_state_t::empty, chunk_state_t::in_use))
        {
            return next_chunk_offset;
        }
    }
}

chunk_offset_t memory_manager_t::allocate_reused_chunk()
{
    // Starting from the first chunk, scan for the first available reused chunk,
    // up to a snapshot of next_available_unused_chunk_offset. If we fail to
    // claim an available chunk, move onto the next one. (If we fail to find or
    // claim any reused chunks, then the caller can allocate a new chunk from
    // unused memory.) Since next_available_unused_chunk_offset can be
    // concurrently advanced, and chunks can also be deallocated behind our scan
    // pointer, this search is best-effort; we could miss a chunk deallocated
    // concurrently with our scan.
    size_t first_unused_chunk_offset = m_metadata->next_available_unused_chunk_offset;
    if (first_unused_chunk_offset != c_first_chunk_offset)
    {
        size_t current_start_offset = c_first_chunk_offset;
        while (current_start_offset < first_unused_chunk_offset)
        {
            size_t found_index = find_first_element(
                m_metadata->chunk_bitmap,
                memory_manager_metadata_t::c_chunk_bitmap_words_size,
                c_chunk_state_bitarray_width,
                common::to_integral(chunk_state_t::empty),
                current_start_offset,
                first_unused_chunk_offset);

            if (found_index == -1)
            {
                break;
            }

            // We found an available chunk, so try to claim it.
            auto available_chunk_offset = static_cast<chunk_offset_t>(found_index);
            if (m_metadata->apply_chunk_transition(
                    available_chunk_offset, chunk_state_t::empty, chunk_state_t::in_use))
            {
                return available_chunk_offset;
            }

            // We failed to claim the chunk at this index, so start the next
            // search after the current position.
            current_start_offset = found_index + 1;
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
        std::cerr << "Reusing chunk " << allocated_chunk_offset << std::endl;
#ifdef DEBUG
        // In debug mode, we write-protect all allocations after writes are
        // complete. (We do this by allocating only on page boundaries and
        // write-protecting the pages used for allocations.) If we do not remove
        // this write protection from the deallocated chunk's pages, then when the
        // chunk is reused, any writes will cause a SIGSEGV signal to be sent to the
        // writing process.
        std::cerr << "allocate_chunk(" << allocated_chunk_offset << "): marking all data pages read/write" << std::endl;
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
        if (allocated_chunk_offset != c_invalid_chunk_offset)
        {
            std::cerr << "Allocating unused chunk " << allocated_chunk_offset << std::endl;
        }
    }

    // At this point, we must either have a valid chunk offset, or we have run out of memory.
    ASSERT_INVARIANT(
        (allocated_chunk_offset != c_invalid_chunk_offset) || (m_metadata->next_available_unused_chunk_offset > c_last_chunk_offset),
        "Chunk allocation cannot fail unless memory is exhausted!");

    return allocated_chunk_offset;
}

// Retires an in-use chunk.
void memory_manager_t::retire_chunk(chunk_offset_t chunk_offset)
{
    std::cerr << "Retiring chunk " << chunk_offset << " in state " << m_metadata->get_current_chunk_state(chunk_offset) << std::endl;

    ASSERT_PRECONDITION(
        m_metadata->get_current_chunk_state(chunk_offset) == chunk_state_t::in_use,
        "A chunk cannot be retired unless it is in use!");

    // This should never fail, because only one thread can own an IN_USE chunk.
    bool success = m_metadata->apply_chunk_transition(
        chunk_offset, chunk_state_t::in_use, chunk_state_t::retired);
    ASSERT_INVARIANT(success, "Retiring an in-use chunk cannot fail!");

    // After transitioning the chunk to RETIRED state, we need to check for
    // emptiness. This is because it's possible that GC tasks freed all
    // allocations while the chunk was still in IN_USE state, so the task that
    // made the last deallocation wasn't able to deallocate the chunk. If the
    // chunk is now empty, then there may be no GC task that will ever try to
    // deallocate it, and its virtual memory could be leaked forever, along with
    // any physical memory that couldn't be eagerly decommitted by GC tasks.

    // REVIEW: We need to instantiate an ad-hoc chunk manager just to check for
    // emptiness. This is a bit unfortunate.
    chunk_manager_t chunk_manager;
    chunk_manager.load(chunk_offset);
    bool should_deallocate_chunk = chunk_manager.is_empty();
    chunk_manager.release();
    if (should_deallocate_chunk)
    {
        deallocate_chunk(chunk_offset);
    }
}

// Marks the chunk as free, and decommits its physical memory.
void memory_manager_t::deallocate_chunk(chunk_offset_t chunk_offset)
{
    ASSERT_PRECONDITION(
        m_metadata->get_current_chunk_state(chunk_offset) == chunk_state_t::retired,
        "A chunk cannot be deallocated unless it is retired!");
#ifdef DEBUG
    // Verify that the deallocated chunk contains no live allocations.
    chunk_manager_t chunk_manager;
    chunk_manager.load(chunk_offset);
    ASSERT_INVARIANT(chunk_manager.is_empty(), "Cannot deallocate a non-empty chunk!");
    chunk_manager.release();
#endif

    // To avoid races with concurrent GC tasks, we only decommit data pages,
    // but leave the metadata pages intact. The metadata pages will be reused
    // when the chunk is reused. If many chunks are deallocated and never
    // subsequently reused, we could revisit this decision.

    // Get starting page address and size of page range in bytes.
    // TODO: We could be a little smarter and only decommit up to the last
    // allocated page, but brute force is simpler and safer for now.
    gaia_offset_t first_data_page_offset = offset_from_chunk_and_slot(chunk_offset, c_first_slot_offset);
    std::cerr << "deallocate_chunk(" << chunk_offset << "): first_data_page_offset: " << first_data_page_offset << std::endl;
    void* data_pages_initial_address = page_address_from_offset(first_data_page_offset);
    std::cerr << "deallocate_chunk(" << chunk_offset << "): data_pages_initial_address: " << data_pages_initial_address << std::endl;

    // MADV_FREE seems like the best fit for our needs, since it allows the OS to lazily reclaim decommitted pages.
    // However, it returns EINVAL when used with MAP_SHARED, so we need to use MADV_REMOVE (which works with memfd objects).
    std::cerr << "deallocate_chunk(" << chunk_offset << "): decommitting all pages in chunk " << chunk_offset << " at address " << data_pages_initial_address << std::endl;
    if (-1 == ::madvise(data_pages_initial_address, c_data_pages_size_bytes, MADV_REMOVE))
    {
        throw_system_error("madvise(MADV_REMOVE) failed!");
    }

    // This could fail if a concurrent GC or compaction task already deallocated
    // the chunk. In that case, the chunk could have already been reused by this
    // point. It shouldn't matter, since we don't decommit metadata pages, and
    // decommitted data pages will be allocated/zeroed on demand.
    bool success = m_metadata->apply_chunk_transition(
        chunk_offset, chunk_state_t::retired, chunk_state_t::empty);
    // This assert will fail if the chunk has already been reused.
    // If concurrent reuse is a problem, then instead of introducing a new
    // transitional state, we could use the shared_lock field and acquire an
    // exclusive lock during deallocation, which allocate_chunk() would need to
    // acquire before transitioning the chunk from the EMPTY state to the IN_USE
    // state. However, such reuse is almost certainly benign as noted above,
    // since we don't decommit any metadata pages, and dirty data pages won't be
    // observable to any client (unless there's a bug).
    ASSERT_INVARIANT(
        success || (m_metadata->get_current_chunk_state(chunk_offset) == chunk_state_t::empty),
        "Chunk reused during deallocation!");
}

chunk_state_t memory_manager_t::get_chunk_state(chunk_offset_t chunk_offset)
{
    return m_metadata->get_current_chunk_state(chunk_offset);
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
