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

chunk_version_t chunk_manager_t::get_version()
{
    return m_metadata->get_chunk_version();
}

chunk_state_t chunk_manager_t::get_state()
{
    return m_metadata->get_chunk_state();
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
}

bool chunk_manager_t::is_slot_allocated(slot_offset_t slot_offset) const
{
    validate_metadata(m_metadata);

    size_t bit_index = slot_to_bit_index(slot_offset);

    // We need to return whether the bit is set in the allocation bitmap and
    // unset in the deallocation bitmap.
    bool was_slot_allocated = is_bit_set(
        m_metadata->allocated_slots_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_words_size,
        bit_index);

    bool was_slot_deallocated = is_bit_set(
        m_metadata->deallocated_slots_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_words_size,
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
        chunk_manager_metadata_t::c_slot_bitmap_words_size,
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

    // We need safe_set_bit_value() because GC tasks deallocate slots, and multiple GC
    // tasks can be concurrently active within a chunk.
    safe_set_bit_value(
        m_metadata->deallocated_slots_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_words_size,
        bit_index, true);
}

bool chunk_manager_t::allocate_chunk()
{
    bool is_allocated = m_metadata->apply_chunk_transition(
        chunk_state_t::empty, chunk_state_t::in_use);

    // If allocation succeeded, update the "allocated chunk bitmap" in the memory manager.
    if (is_allocated)
    {
        gaia::db::get_memory_manager()->update_chunk_allocation_status(m_chunk_offset, true);
    }

    return is_allocated;
}

void chunk_manager_t::retire_chunk(chunk_version_t version)
{
    // This transition must succeed because only the owning thread can
    // legally transition the chunk from IN_USE to RETIRED.
    bool transition_succeeded = m_metadata->apply_chunk_transition_from_version(
        version, chunk_state_t::in_use, chunk_state_t::retired);
    ASSERT_INVARIANT(transition_succeeded, "Transition from IN_USE to RETIRED cannot fail!");
    // If the retired chunk is empty, deallocate it.
    try_deallocate_chunk(version);
}

bool chunk_manager_t::try_deallocate_chunk(chunk_version_t initial_version)
{
    // If this method is called without a valid initial version, then the caller
    // must be an owning thread and the chunk must be in IN_USE state.
    ASSERT_PRECONDITION(
        (initial_version != c_invalid_chunk_version) || m_metadata->get_chunk_state() == chunk_state_t::in_use,
        "Chunk must be in IN_USE state if initial version is not supplied!");

    if (!is_empty(initial_version))
    {
        return false;
    }

    bool transition_succeeded = m_metadata->apply_chunk_transition_from_version(
        initial_version, chunk_state_t::retired, chunk_state_t::deallocating);

    // If this method is called without a valid initial version, then concurrent
    // reuse should be impossible.
    ASSERT_INVARIANT(
        transition_succeeded || (initial_version != c_invalid_chunk_version),
        "Transition from RETIRED to DEALLOCATING failed, but no initial version was supplied!");

    if (!transition_succeeded)
    {
        return false;
    }

    // If we successfully transitioned the chunk to DEALLOCATING state, then it
    // cannot be reused until we transition it to EMPTY state, so we can safely
    // decommit all data pages.
    decommit_data_pages();

    // This transition must succeed because only the deallocating thread can
    // legally transition the chunk from DEALLOCATING to EMPTY.
    transition_succeeded = m_metadata->apply_chunk_transition_from_version(
        initial_version, chunk_state_t::deallocating, chunk_state_t::empty);
    ASSERT_INVARIANT(transition_succeeded, "Transition from DEALLOCATING to EMPTY cannot fail!");

    // Finally, update the "free chunk bitmap" in the memory manager.
    gaia::db::get_memory_manager()->update_chunk_allocation_status(m_chunk_offset, false);

    return true;
}

void chunk_manager_t::decommit_data_pages()
{
    ASSERT_PRECONDITION(
        m_metadata->get_chunk_state() == chunk_state_t::deallocating,
        "Chunk must be in DEALLOCATING state to decommit data pages!");

    // Get address of first data page.
    gaia_offset_t first_data_page_offset = offset_from_chunk_and_slot(m_chunk_offset, c_first_slot_offset);
    void* first_data_page_address = page_address_from_offset(first_data_page_offset);

    // In debug mode, we must remove any write protection from the chunk, or
    // madvise(MADV_REMOVE) will fail with EACCES.
#ifdef DEBUG
    if (-1 == ::mprotect(first_data_page_address, c_data_pages_size_bytes, PROT_READ | PROT_WRITE))
    {
        throw_system_error("mprotect(PROT_READ|PROT_WRITE) failed!");
    }
#endif

    // MADV_FREE seems like the best fit for our needs, since it allows the OS to lazily reclaim decommitted pages.
    // However, it returns EINVAL when used with MAP_SHARED, so we need to use MADV_REMOVE (which works with memfd objects).
    // According to the manpage, madvise(MADV_REMOVE) is equivalent to fallocate(FALLOC_FL_PUNCH_HOLE).
    if (-1 == ::madvise(first_data_page_address, c_data_pages_size_bytes, MADV_REMOVE))
    {
        throw_system_error("madvise(MADV_REMOVE) failed!");
    }
}

bool chunk_manager_t::is_empty(chunk_version_t initial_version)
{
    // This is a best-effort non-atomic scan; it's possible that some page
    // previously observed to be non-empty will be empty by the end of the scan.
    // However, if `initial_version` is set to a valid version (as it should be
    // when called from a non-owning thread), then returning success indicates
    // that the chunk was not reused during the scan.

    // Loop over both bitmaps in parallel, XORing each pair of words to
    // determine if there are any live allocations in that word.
    for (size_t word_index = 0; word_index < chunk_manager_metadata_t::c_slot_bitmap_words_size; ++word_index)
    {
        uint64_t allocation_word = m_metadata->allocated_slots_bitmap[word_index];
        uint64_t deallocation_word = m_metadata->deallocated_slots_bitmap[word_index];

        // If we're not doing the version check, then assume that the chunk
        // can't be concurrently reused during the scan, so assert that expected
        // invariants hold.
        if (initial_version == c_invalid_chunk_version)
        {
            // The bits set in the deallocation bitmap word must be a subset of the
            // bits set in the allocation bitmap word.
            ASSERT_INVARIANT(
                (allocation_word | deallocation_word) == allocation_word,
                "All bits set in the deallocation bitmap must be set in the allocation bitmap!");
        }

        // If some bits set in the allocation word are not set in the deallocation word,
        // then this page contains live allocations, otherwise it is empty.
        bool is_page_empty = (allocation_word ^ deallocation_word == 0);
        bool has_version_changed = (initial_version != m_metadata->get_chunk_version());

        // REVIEW: do we need a distinct return value for failed version check?
        if (!is_page_empty || has_version_changed)
        {
            return false;
        }
    }
    return true;
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
