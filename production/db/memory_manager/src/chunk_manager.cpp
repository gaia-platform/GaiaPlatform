/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "chunk_manager.hpp"

#include <sys/mman.h>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"

#include "bitmap.hpp"
#include "db_helpers.hpp"
#include "db_shared_data.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

using namespace gaia::common;
using scope_guard::make_scope_guard;

static void validate_metadata(chunk_manager_metadata_t* metadata)
{
    ASSERT_PRECONDITION(metadata != nullptr, "Chunk manager was not initialized!");
}

void chunk_manager_t::initialize_internal(
    chunk_offset_t chunk_offset, bool initialize_memory)
{
    m_chunk_offset = chunk_offset;

    // Map the metadata information for quick reference.
    char* base_address = reinterpret_cast<char*>(gaia::db::get_data()->objects);
    void* metadata_address = base_address + (chunk_offset * c_chunk_size_bytes);
    m_metadata = static_cast<chunk_manager_metadata_t*>(metadata_address);

    if (initialize_memory)
    {
        m_metadata->clear();
    }
}

void chunk_manager_t::initialize(chunk_offset_t chunk_offset)
{
    bool initialize_memory = true;
    initialize_internal(chunk_offset, initialize_memory);
}

void chunk_manager_t::load(chunk_offset_t chunk_offset)
{
    bool initialize_memory = false;
    initialize_internal(chunk_offset, initialize_memory);
}

chunk_offset_t chunk_manager_t::release()
{
    chunk_offset_t chunk_offset = m_chunk_offset;
    m_chunk_offset = c_invalid_chunk_offset;
    m_metadata = nullptr;
    return chunk_offset;
}

gaia_offset_t chunk_manager_t::allocate(
    size_t allocation_bytes_size)
{
    // If we are uninitialized, then the caller must initialize us with a new chunk.
    if (!initialized())
    {
        return c_invalid_gaia_offset;
    }

    ASSERT_PRECONDITION(allocation_bytes_size > 0, "Requested allocation size cannot be 0!");

    // Check before converting to slot units to avoid overflow.
    ASSERT_PRECONDITION(
        allocation_bytes_size <= c_max_allocation_slots_size * c_slot_size_bytes,
        "Requested allocation size exceeds maximum allocation size of 64KB!");

    validate_metadata(m_metadata);

    // Calculate allocation size in slot units.
#ifdef DEBUG
    // Round up allocation to a page so we can mprotect() it.
    size_t allocation_pages_size = (allocation_bytes_size + c_page_size_bytes - 1) / c_page_size_bytes;
    size_t allocation_slots_size = allocation_pages_size * (c_page_size_bytes / c_slot_size_bytes);
#else
    size_t allocation_slots_size = (allocation_bytes_size + c_slot_size_bytes - 1) / c_slot_size_bytes;
#endif
    // Ensure that the new allocation doesn't overflow the chunk.
    if (m_metadata->min_unallocated_slot_offset() + allocation_slots_size > c_last_slot_offset)
    {
        return c_invalid_gaia_offset;
    }

    // Ensure that allocation metadata is consistent if an exception is thrown
    // while it is being updated.
    auto sync_metadata = make_scope_guard([&]() {
        m_metadata->synchronize_allocation_metadata();
    });

    // Now update the atomic allocation metadata.
    m_metadata->update_last_allocation_metadata(allocation_slots_size);

    // Finally, update the allocation bitmap (order is important for crash-consistency).
    slot_offset_t allocated_slot = m_metadata->max_allocated_slot_offset();
    mark_slot_allocated(allocated_slot);
    ASSERT_INVARIANT(
        is_slot_allocated(allocated_slot),
        "Slot just marked allocated must be visible as allocated!");

    gaia_offset_t offset = offset_from_chunk_and_slot(m_chunk_offset, allocated_slot);
    return offset;
}

void chunk_manager_t::deallocate(gaia_offset_t offset)
{
    slot_offset_t deallocated_slot = slot_from_offset(offset);

    // It is illegal to deallocate the same object twice.
    ASSERT_PRECONDITION(
        is_slot_allocated(deallocated_slot),
        "Only an allocated object can be deallocated!");

    // Update the deallocation bitmap.
    mark_slot_deallocated(deallocated_slot);

    // Decommit any eligible physical pages.
    decommit_physical_pages_unused_after_object_deallocation(deallocated_slot);
}

void chunk_manager_t::decommit_physical_pages_unused_after_object_deallocation(
    slot_offset_t deallocated_slot_offset)
{
    // Eager decommit of physical pages (i.e., as soon as a page contains no
    // live allocations) is best-effort: if a page contains the current
    // allocation boundary, then it cannot be decommitted. The allocation
    // boundary check is conservative, because it may be invalid by the time the
    // deallocating thread would have decommitted the page, but it is safe
    // because slots are allocated monotonically. If an empty page cannot be
    // decommitted because of the allocation boundary check, it will be leaked
    // until the entire chunk is deallocated after its last object is
    // deallocated. This is not expected to be an issue in practice.
    //
    // To determine if this allocation continues into consecutive pages, we need
    // to determine whether it is the last set bit in its word in the allocation
    // bitmap. If so, we need to scan forward to the next set bit in the
    // allocation bitmap to find the end of this allocation (or to
    // min_unallocated_slot_offset if it is the last set bit in the allocation
    // bitmap). We can decommit any pages used by this allocation, except for
    // possibly the last used page, which we need to check for other live
    // allocations.
    //
    // To determine if a previous live allocation continues into this
    // allocation's first page, we need to scan backward to the last preceding
    // set bit in the allocation bitmap, and check if its corresponding bit is
    // set in the deallocation bitmap. If not, then we cannot decommit the page.
    //
    // We combine these page ranges (the current page, if no live allocations
    // are using it, any consecutive pages wholly contained in this allocation,
    // and the final page of this allocation, if no live allocations are using
    // it) into a single contiguous range, which we submit to
    // madvise(MADV_FREE).

    // First, test whether the first page of this allocation is empty.
    size_t deallocated_slot_page_index = slot_to_page_index(deallocated_slot_offset);
    size_t deallocated_slot_in_page_index = slot_to_word_index(deallocated_slot_offset);
    bool is_first_allocated_page_empty = is_page_empty(deallocated_slot_page_index);

    // Next, test whether this allocation is the last set bit in its word in the allocation bitmap.
    // If so, then it may continue into consecutive pages (unless the first bit in the next word is set).
    uint64_t first_page_allocations = m_metadata->allocated_slots_bitmap[deallocated_slot_page_index];
    size_t last_allocation_in_page_index = find_last_set_bit_in_word(first_page_allocations);
    bool is_last_allocation_in_first_page = (deallocated_slot_in_page_index == last_allocation_in_page_index);

    // Finally, test whether the first slot in this allocation's first page has its bit set.
    // If not, there is another allocation continuing into the first page.
    bool first_page_has_leading_allocation = !(first_page_allocations & 1);
    // This cannot be the first page in the chunk, or there would not be a leading allocation.
    ASSERT_INVARIANT(
        !first_page_has_leading_allocation || (deallocated_slot_page_index > 0),
        "The first page in a chunk cannot contain a leading allocation!");

    // If the first page is empty and contains a leading allocation, then
    // determine if the allocation corresponding to the last set bit in a
    // previous word is still live. If not, we can decommit the first page.

    bool can_decommit_first_allocated_page = is_first_allocated_page_empty;
    if (is_first_allocated_page_empty && first_page_has_leading_allocation)
    {
        can_decommit_first_allocated_page = false;

        // Scan backward to the last set bit in previous words.
        // (By the previous assert, this is not the first page in the chunk.)
        for (size_t page_index = deallocated_slot_page_index - 1;
             page_index < deallocated_slot_page_index - 1; --page_index)
        {
            uint64_t allocation_word = m_metadata->allocated_slots_bitmap[page_index];
            if (allocation_word == 0)
            {
                // We can't get all the way to the start of the allocation bitmap without finding a set bit.
                ASSERT_INVARIANT(page_index != 0, "A leading allocation must have an allocation bit set!");
                continue;
            }

            size_t last_allocation_bit = find_last_set_bit_in_word(allocation_word);
            uint64_t deallocation_word = m_metadata->deallocated_slots_bitmap[page_index];
            uint64_t mask = 1ULL << last_allocation_bit;
            // If the leading allocation is deallocated, then we can decommit the first page.
            can_decommit_first_allocated_page = (deallocation_word & mask);
            break;
        }
    }

    // If this is the last allocation in its page, we need to find all the pages
    // contained within its allocation, by scanning forward to the next set bit
    // in the allocation bitmap, until we reach the first unallocated slot. We
    // can decommit all pages entirely contained within this allocation, and we
    // can decommit the first and last pages as well, if they contain no live
    // allocations.

    size_t trailing_pages_to_decommit_count = 0;
    if (is_last_allocation_in_first_page)
    {
        // We can scan only up to the allocation boundary.
        size_t min_unallocated_slot_page_index = slot_to_page_index(
            m_metadata->min_unallocated_slot_offset());

        // Scan forward to the first set bit in consecutive words.
        for (size_t page_index = deallocated_slot_page_index + 1;
             page_index <= min_unallocated_slot_page_index; ++page_index)
        {
            uint64_t allocation_word = m_metadata->allocated_slots_bitmap[page_index];
            if (allocation_word == 0)
            {
                // This page is contained completely within our allocation, so add it to the count.
                ++trailing_pages_to_decommit_count;
                continue;
            }

            // If the first set bit in this word is at the beginning of the page,
            // then this page is not part of our allocation.
            if (find_first_set_bit_in_word(allocation_word) == 0)
            {
                break;
            }

            // If there are no live allocations in the final page, then we can decommit it as well.
            if (is_page_empty(page_index))
            {
                ++trailing_pages_to_decommit_count;
            }
            break;
        }
    }

    // Now actually decommit eligible pages.
    if (can_decommit_first_allocated_page || (trailing_pages_to_decommit_count > 0))
    {
        size_t start_page_index_to_decommit = deallocated_slot_page_index;
        size_t pages_to_decommit_count = 1;
        if (!can_decommit_first_allocated_page)
        {
            ++start_page_index_to_decommit;
            --pages_to_decommit_count;
        }
        pages_to_decommit_count += trailing_pages_to_decommit_count;

        // Get starting page address and size of page range in bytes.
        slot_offset_t first_page_slot = page_index_to_first_slot_in_page(start_page_index_to_decommit);
        gaia_offset_t first_page_offset = offset_from_chunk_and_slot(m_chunk_offset, first_page_slot);
        void* pages_to_decommit_initial_address = page_address_from_offset(first_page_offset);
        size_t pages_to_decommit_size_bytes = pages_to_decommit_count * c_page_size_bytes;

        // MADV_FREE seems like the best fit for our needs, since it allows the OS to lazily reclaim decommitted pages.
        // However, it returns EINVAL when used with MAP_SHARED, so we need to use MADV_REMOVE (which works with memfd objects).
        if (-1 == ::madvise(pages_to_decommit_initial_address, pages_to_decommit_size_bytes, MADV_REMOVE))
        {
            throw_system_error("madvise(MADV_FREE) failed!");
        }
    }
}

bool chunk_manager_t::is_slot_allocated(slot_offset_t slot_offset) const
{
    validate_metadata(m_metadata);

    size_t bit_index = slot_to_bit_index(slot_offset);

    // We need to return whether the bit is set in the allocation bitmap and
    // unset in the deallocation bitmap.
    bool was_slot_allocated = is_bit_set(
        m_metadata->allocated_slots_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_size,
        bit_index);

    bool was_slot_deallocated = is_bit_set(
        m_metadata->deallocated_slots_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_size,
        bit_index);

    bool is_slot_allocated = (was_slot_allocated && !was_slot_deallocated);

    ASSERT_INVARIANT(
        was_slot_allocated || !was_slot_deallocated,
        "It is illegal for a slot to be marked in the deallocation bitmap but not in the allocation bitmap!");

    return is_slot_allocated;
}

void chunk_manager_t::mark_slot_allocated(slot_offset_t slot_offset)
{
    validate_metadata(m_metadata);
    ASSERT_PRECONDITION(
        slot_offset >= c_first_slot_offset && slot_offset <= c_last_slot_offset,
        "Slot offset passed to try_mark_slot_used_status() is out of bounds");

    // is_slot_allocated() also checks that the deallocation bit is not set if
    // the allocation bit is not set.
    ASSERT_PRECONDITION(
        !is_slot_allocated(slot_offset),
        "Slot cannot be allocated multiple times!");

    size_t bit_index = slot_to_bit_index(slot_offset);

    // // We don't need a CAS because only the owning thread can allocate a slot.
    set_bit_value(
        m_metadata->allocated_slots_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_size,
        bit_index,
        true);
}

void chunk_manager_t::mark_slot_deallocated(slot_offset_t slot_offset)
{
    validate_metadata(m_metadata);
    ASSERT_PRECONDITION(
        slot_offset >= c_first_slot_offset && slot_offset <= c_last_slot_offset,
        "Slot offset passed to try_mark_slot_used_status() is out of bounds");

    // is_slot_allocated() also checks that the deallocation bit is not set if
    // the allocation bit is not set.
    ASSERT_PRECONDITION(
        is_slot_allocated(slot_offset),
        "Slot cannot be deallocated unless it is first allocated!");

    size_t bit_index = slot_to_bit_index(slot_offset);

    // We need a CAS loop because GC tasks deallocate slots, and multiple GC
    // tasks can be concurrently active within a chunk.
    while (!try_set_bit_value(
        m_metadata->deallocated_slots_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_size,
        bit_index,
        true))
        ;
}

bool chunk_manager_t::is_empty()
{
    // This is a best-effort non-atomic scan; it's possible that some word
    // previously observed to be non-empty will be empty by the end of the scan.
    // However, the GC task that deallocates the last object within a chunk
    // *must* observe the entire bitmap to be empty after it has set its
    // deallocation bit, so a non-empty chunk cannot be leaked. It is possible
    // for multiple GC tasks to observe the chunk to be empty and attempt to
    // deallocate the chunk, but only one can succeed in transitioning it to the
    // EMPTY state, and physical page deallocation is idempotent and threadsafe.

    // Loop over both bitmaps in parallel, XORing each pair of words to
    // determine if there are any live allocations in that word.
    for (size_t word_index = 0; word_index < chunk_manager_metadata_t::c_slot_bitmap_size; ++word_index)
    {
        // Each word in the bitmaps corresponds to a page of data.
        if (!is_page_empty(word_index))
        {
            return false;
        }
    }
    return true;
}

bool chunk_manager_t::is_page_empty(size_t page_index)
{
    uint64_t allocation_word = m_metadata->allocated_slots_bitmap[page_index];
    uint64_t deallocation_word = m_metadata->deallocated_slots_bitmap[page_index];

    // The bits set in the deallocation bitmap word must be a subset of the
    // bits set in the allocation bitmap word.
    ASSERT_INVARIANT(
        (allocation_word | deallocation_word) == allocation_word,
        "All bits set in the deallocation bitmap must be set in the allocation bitmap!");

    // If some bits set in the allocation word are not set in the deallocation word,
    // then this word contains live allocations, otherwise it is empty.
    return (allocation_word ^ deallocation_word) == 0;
}

gaia_offset_t chunk_manager_t::last_allocated_offset()
{
    // This is expected to be called from a server session thread cleaning up
    // after a crash, so we expect allocation metadata to be possibly
    // inconsistent.
    m_metadata->synchronize_allocation_metadata();

    slot_offset_t max_allocated_slot_offset = m_metadata->max_allocated_slot_offset();
    if (max_allocated_slot_offset == c_invalid_slot_offset)
    {
        return c_invalid_gaia_offset;
    }
    return offset_from_chunk_and_slot(m_chunk_offset, max_allocated_slot_offset);
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
