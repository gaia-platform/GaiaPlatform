/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "chunk_manager.hpp"

#include <iostream>
#include <thread>

#include <sys/mman.h>

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/common/bitmap.hpp"
#include "gaia_internal/common/debug_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"

#include "memory_helpers.hpp"
#include "memory_manager.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

using namespace gaia::common;
using namespace gaia::common::bitmap;
using scope_guard::make_scope_guard;

void chunk_manager_t::initialize_internal(
    chunk_offset_t chunk_offset, bool initialize_memory)
{
    m_chunk_offset = chunk_offset;

    // Map the metadata information for quick reference.
    char* base_address = reinterpret_cast<char*>(gaia::db::get_data()->objects);
    void* metadata_address = base_address + (chunk_offset * c_chunk_size_in_bytes);
    m_metadata = static_cast<chunk_manager_metadata_t*>(metadata_address);

    if (initialize_memory)
    {
        m_metadata->clear();
    }
}

void chunk_manager_t::initialize(chunk_offset_t chunk_offset)
{
    validate_uninitialized();
    bool initialize_memory = true;
    initialize_internal(chunk_offset, initialize_memory);
}

void chunk_manager_t::load(chunk_offset_t chunk_offset)
{
    validate_uninitialized();
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

chunk_version_t chunk_manager_t::get_version() const
{
    validate_initialized();
    return m_metadata->get_chunk_version();
}

chunk_state_t chunk_manager_t::get_state() const
{
    validate_initialized();
    return m_metadata->get_chunk_state();
}

gaia_offset_t chunk_manager_t::allocate(
    size_t allocation_size_in_bytes)
{
    // If we are uninitialized, then the caller must initialize us with a new chunk.
    if (!initialized())
    {
        return c_invalid_gaia_offset;
    }

    ASSERT_PRECONDITION(
        get_state() == chunk_state_t::in_use,
        "Objects can only be allocated from a chunk in the IN_USE state!");

    size_t allocation_size_in_slots = calculate_allocation_size_in_slots(allocation_size_in_bytes);

    // Ensure that the new allocation doesn't overflow the chunk.
    if (m_metadata->min_unallocated_slot_offset() + allocation_size_in_slots > c_last_slot_offset)
    {
        return c_invalid_gaia_offset;
    }

    // Now update the atomic allocation metadata.
    m_metadata->update_last_allocation_metadata(allocation_size_in_slots);

    // Update the allocation bitmap (order is important for crash-consistency).
    slot_offset_t allocated_slot = m_metadata->max_allocated_slot_offset();
    mark_slot_allocated(allocated_slot);

    DEBUG_ASSERT_INVARIANT(
        is_slot_allocated(allocated_slot),
        "Slot just marked allocated must be visible as allocated!");

    gaia_offset_t offset = offset_from_chunk_and_slot(m_chunk_offset, allocated_slot);
    return offset;
}

void chunk_manager_t::deallocate(gaia_offset_t offset)
{
    ASSERT_PRECONDITION(
        (get_state() == chunk_state_t::in_use) || (get_state() == chunk_state_t::retired),
        "Objects can only be deallocated from a chunk in the IN_USE or RETIRED state!");

    slot_offset_t deallocated_slot = slot_from_offset(offset);

    // It is illegal to deallocate the same object twice.
    DEBUG_ASSERT_PRECONDITION(
        is_slot_allocated(deallocated_slot),
        "Only an allocated object can be deallocated!");

    // Update the deallocation bitmap.
    mark_slot_deallocated(deallocated_slot);
}

bool chunk_manager_t::is_slot_allocated(slot_offset_t slot_offset) const
{
    validate_initialized();

    size_t bit_index = slot_to_bit_index(slot_offset);

    // We need to return whether the bit is set in the allocation bitmap and
    // unset in the deallocation bitmap.
    bool was_slot_allocated = is_bit_set(
        m_metadata->allocated_slots_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_size_in_words,
        bit_index);

    bool was_slot_deallocated = is_bit_set(
        m_metadata->deallocated_slots_bitmap,
        chunk_manager_metadata_t::c_slot_bitmap_size_in_words,
        bit_index);

    ASSERT_INVARIANT(
        was_slot_allocated || !was_slot_deallocated,
        "It is illegal for a slot to be marked in the deallocation bitmap but not in the allocation bitmap!");

    bool is_slot_allocated = (was_slot_allocated && !was_slot_deallocated);

    return is_slot_allocated;
}

void chunk_manager_t::mark_slot(slot_offset_t slot_offset, bool is_allocating)
{
    validate_initialized();

    ASSERT_PRECONDITION(
        slot_offset >= c_first_slot_offset && slot_offset <= c_last_slot_offset,
        "Slot offset passed to mark_slot() is out of bounds");

    // is_slot_allocated() also checks that the deallocation bit is not set if
    // the allocation bit is not set.
    DEBUG_ASSERT_PRECONDITION(
        is_allocating != is_slot_allocated(slot_offset),
        is_allocating
            ? "Slot cannot be allocated multiple times!"
            : "Slot cannot be deallocated unless it is first allocated!");

    size_t bit_index = slot_to_bit_index(slot_offset);

    // We need safe_set_bit_value() only for deallocation, because GC tasks
    // deallocate slots, and multiple GC tasks can be concurrently active within
    // a chunk, but only the owning thread can allocate slots in a chunk.
    if (is_allocating)
    {
        common::bitmap::set_bit_value(
            m_metadata->allocated_slots_bitmap,
            chunk_manager_metadata_t::c_slot_bitmap_size_in_words,
            bit_index,
            true);
    }
    else
    {
        common::bitmap::safe_set_bit_value(
            m_metadata->deallocated_slots_bitmap,
            chunk_manager_metadata_t::c_slot_bitmap_size_in_words,
            bit_index,
            true);
    }
}

void chunk_manager_t::mark_slot_allocated(slot_offset_t slot_offset)
{
    bool is_allocating = true;
    mark_slot(slot_offset, is_allocating);
}

void chunk_manager_t::mark_slot_deallocated(slot_offset_t slot_offset)
{
    bool is_allocating = false;
    mark_slot(slot_offset, is_allocating);
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

bool chunk_manager_t::is_empty(chunk_version_t initial_version) const
{
    // This is a best-effort non-atomic scan; it's possible that some page
    // previously observed to be non-empty will be empty by the end of the scan.
    // However, we check after every read that the current version is still
    // `initial_version`, so if this method returns success, the caller knows
    // that the chunk was not reused during the scan.

    // Loop over both bitmaps in parallel, XORing each pair of words to
    // determine if there are any initial slots of live allocations in that
    // word.
    for (size_t word_index = 0; word_index < chunk_manager_metadata_t::c_slot_bitmap_size_in_words; ++word_index)
    {
        uint64_t allocation_word = m_metadata->allocated_slots_bitmap[word_index];
        uint64_t deallocation_word = m_metadata->deallocated_slots_bitmap[word_index];

        // If the version changed, then the chunk was concurrently reused during
        // the scan, so abort without checking invariants.
        // NB: It's crucial that the version is checked immediately _after_ we
        // have read any values that might have concurrently changed, but
        // _before_ we do anything with them. If we checked the version before
        // the read, we wouldn't know if the chunk had been concurrently reused
        // before or during the read, so the data we just read might be invalid
        // and we couldn't safely check invariants. If the version check
        // succeeds, we know that the chunk wasn't reused before or during the
        // read, so the data we read was valid *at the time we read it*, even
        // though it might be invalid now. But if it is invalid (i.e., the
        // version has changed), we'll find out when we try to transition the
        // chunk to a new state using our now-outdated version.
        // REVIEW: do we need a distinct return value for failed
        // version check?
        chunk_version_t current_version = m_metadata->get_chunk_version();
        if (current_version != initial_version)
        {
            return false;
        }

        // The bits set in the deallocation bitmap word must be a subset of the
        // bits set in the allocation bitmap word.
        ASSERT_INVARIANT(
            (allocation_word | deallocation_word) == allocation_word,
            "All bits set in the deallocation bitmap must be set in the allocation bitmap!");

        // If some bits set in the allocation word are not set in the
        // deallocation word, then this page contains the initial slots of live
        // allocations, otherwise the only live allocations it can contain must
        // be continued from a previous page. But if no pages contain the
        // initial slots of live allocations, then the entire chunk must be
        // empty.
        if ((allocation_word ^ deallocation_word) != 0)
        {
            return false;
        }
    }

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
    if (-1 == ::mprotect(first_data_page_address, c_chunk_data_pages_size_in_bytes, PROT_READ | PROT_WRITE))
    {
        throw_system_error("mprotect(PROT_READ|PROT_WRITE) failed!");
    }
#endif

    // MADV_FREE seems like the best fit for our needs, since it allows the OS to lazily reclaim decommitted pages.
    // However, it returns EINVAL when used with MAP_SHARED, so we need to use MADV_REMOVE (which works with memfd objects).
    // According to the manpage, madvise(MADV_REMOVE) is equivalent to fallocate(FALLOC_FL_PUNCH_HOLE).
    if (-1 == ::madvise(first_data_page_address, c_chunk_data_pages_size_in_bytes, MADV_REMOVE))
    {
        throw_system_error("madvise(MADV_REMOVE) failed!");
    }
}

bool chunk_manager_t::try_deallocate_chunk(chunk_version_t initial_version)
{
    if (!is_empty(initial_version))
    {
        return false;
    }

    // This transition may fail if the chunk is still in the IN_USE state, or if
    // another thread concurrently transitioned it from RETIRED to DEALLOCATING,
    // or if the chunk was concurrently reused.
    bool transition_succeeded = m_metadata->apply_chunk_transition_from_version(
        initial_version, chunk_state_t::retired, chunk_state_t::deallocating);

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

    // Finally, update the "allocated chunk bitmap" in the memory manager.
    gaia::db::get_memory_manager()->update_chunk_allocation_status(m_chunk_offset, false);

    return true;
}

gaia_offset_t chunk_manager_t::last_allocated_offset()
{
    // This is expected to be called from a server session thread cleaning up
    // after a crash, so we expect allocation metadata to be possibly
    // inconsistent.
    m_metadata->synchronize_allocation_metadata();

    slot_offset_t max_allocated_slot_offset = m_metadata->max_allocated_slot_offset();
    if (!max_allocated_slot_offset.is_valid())
    {
        return c_invalid_gaia_offset;
    }
    return offset_from_chunk_and_slot(m_chunk_offset, max_allocated_slot_offset);
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
