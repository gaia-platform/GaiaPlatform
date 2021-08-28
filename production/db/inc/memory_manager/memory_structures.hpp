/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <atomic>

#include "gaia_internal/common/enum_helpers.hpp"
#include "gaia_internal/common/inline_shared_lock.hpp"

#include "bitmap.hpp"
#include "db_internal_types.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

// A memory manager's metadata information.
struct memory_manager_metadata_t
{
    // A 256GB memory space can store 2^16 4MB chunks.
    // We'll reserve a full chunk for metadata, because chunks are the smallest
    // allocation units at memory manager level.
    // There are 4 chunk states defined, so we need 2 bits/chunk.
    // Therefore our bitmap contains 2*2^16 bits = 16KB, or 2048 64-bit words.
    static constexpr size_t c_chunk_bitmap_words_size{
        (c_chunk_state_bitarray_width * (1ULL << (CHAR_BIT * sizeof(chunk_offset_t)))) / c_uint64_bit_count};

    // This is a physical array of 64-bit words, storing a logical array of
    // 2-bit elements, each holding the current state of the chunk whose
    // chunk_offset is the logical array index.
    std::atomic<uint64_t> chunk_bitmap[c_chunk_bitmap_words_size];

    // As we keep allocating memory, the available contiguous virtual memory
    // region will keep shrinking. We'll use this offset to track the next
    // unallocated chunk.
    std::atomic<chunk_offset_t> next_available_unused_chunk_offset{};

    // Just in case atomic types smaller than a word aren't lock-free.
    static_assert(std::atomic<chunk_offset_t>::is_always_lock_free);

    // These reserved variables account for how much space is remaining unused
    // out of the space we reserved for this metadata structure.
    // The reserved array covers the bulk of the space as a multiple of 8B words
    // and any remainder smaller than 8B is covered by individual variables.
    uint32_t reserved1;
    uint32_t reserved2;
    uint64_t reserved[c_chunk_size_bytes / sizeof(uint64_t) - c_chunk_bitmap_words_size - 2];

    inline memory_manager_metadata_t();

    inline void clear();

    struct valid_chunk_transition_t
    {
        chunk_state_t old_state;
        chunk_state_t new_state;
    };

    static inline constexpr valid_chunk_transition_t c_valid_chunk_transitions[] = {
        // ALLOCATE
        {chunk_state_t::empty, chunk_state_t::in_use},
        // GC
        {chunk_state_t::in_use, chunk_state_t::in_use},
        // SEAL
        {chunk_state_t::in_use, chunk_state_t::retired},
        // GC
        {chunk_state_t::retired, chunk_state_t::retired},
        // GC
        {chunk_state_t::retired, chunk_state_t::empty},
        // SCHEDULE_FOR_COMPACTION
        {chunk_state_t::retired, chunk_state_t::pending_compaction},
        // COMPACT
        {chunk_state_t::pending_compaction, chunk_state_t::empty},
    };

    inline bool apply_chunk_transition(
        chunk_offset_t chunk_offset, chunk_state_t expected_state, chunk_state_t desired_state);

    inline chunk_state_t get_current_chunk_state(chunk_offset_t chunk_offset)
    {
        uint64_t state_bits = get_element_at_index(
            chunk_bitmap, c_chunk_bitmap_words_size,
            c_chunk_state_bitarray_width, chunk_offset);

        return chunk_state_t{state_bits};
    }
};

// Constants for the range of available chunks within our memory.
constexpr chunk_offset_t c_first_chunk_offset = sizeof(memory_manager_metadata_t) / c_chunk_size_bytes;
constexpr chunk_offset_t c_last_chunk_offset = -1;

inline memory_manager_metadata_t::memory_manager_metadata_t()
    : next_available_unused_chunk_offset(c_first_chunk_offset)
{
}

inline void memory_manager_metadata_t::clear()
{
    next_available_unused_chunk_offset = c_first_chunk_offset;
    std::fill(chunk_bitmap, chunk_bitmap + c_chunk_bitmap_words_size, 0);
}

inline bool memory_manager_metadata_t::apply_chunk_transition(
    chunk_offset_t chunk_offset, chunk_state_t expected_state, chunk_state_t desired_state)
{
    ASSERT_PRECONDITION(
        chunk_offset >= c_first_chunk_offset && chunk_offset <= c_last_chunk_offset,
        "Chunk offset passed to apply_chunk_transition() is out of bounds");

    // Validate the proposed transition.
    bool is_valid_transition = false;
    for (auto t : memory_manager_metadata_t::c_valid_chunk_transitions)
    {
        if (t.old_state == expected_state && t.new_state == desired_state)
        {
            is_valid_transition = true;
            break;
        }
    }
    ASSERT_PRECONDITION(is_valid_transition, "No valid transition found!");

    // Conditionally set the chunk's state to desired_state, if its initial
    // state is expected_state.
    return conditional_set_element_at_index(
        chunk_bitmap, c_chunk_bitmap_words_size, c_chunk_state_bitarray_width, chunk_offset,
        common::to_integral(expected_state), common::to_integral(desired_state));
}

// This assert could be edited if we need to use more memory for the metadata.
static_assert(
    sizeof(memory_manager_metadata_t) == c_chunk_size_bytes,
    "The size of memory_manager_metadata_t is expected to be 4MB!");

// This assert should never need to be changed, unless the design is radically updated.
// This is because a chunk is the smallest allocation unit of a memory manager,
// so it does not make sense for metadata to only use a chunk partially - we'll just
// reserve any remaining space in a chunk for future use.
static_assert(
    sizeof(memory_manager_metadata_t) % c_chunk_size_bytes == 0,
    "The size of memory_manager_metadata_t is expected to be a multiple of the chunk size!");

// A chunk manager's metadata information.
struct chunk_manager_metadata_t
{
    // A 4MB chunk can store 2^16 64B slots.
    // A bitmap for 2^16 slots takes 8kB, or the space of 128 slots.
    // Because the bitmap does not need to track those 128 slots, that frees 16B.
    // In terms of 64b words, the bitmap only needs 1024 - 2 such words for its tracking.
    // The 2 words can be used to store additional metadata.
    static constexpr size_t c_slot_bitmap_size = 1024 - 2;

    // This lock is used to enforce mutual exclusion between a compaction task
    // and any GC tasks. We allow any number of GC tasks to be concurrently
    // active in the same chunk, but no GC tasks can be active in a chunk while
    // it is being compacted. The high bit of the lock word is used as the
    // exclusive lock for compaction, while the remaining bits are used as a
    // reference count to track concurrent GC tasks.
    common::inline_shared_lock shared_lock;

    static_assert(
        sizeof(common::inline_shared_lock) == sizeof(uint64_t),
        "Expected inline_shared_lock to occupy 8 bytes!");

    // We group our "last allocation metadata" logical variables into an 8-byte
    // struct, so we can set them all in a single atomic write, which is
    // necessary for crash-consistency. Given that these 3 variables are always
    // consistent, we can recover from arbitrary crashes by determining whether
    // the allocation bitmap is out-of-date WRT the "last allocation metadata",
    // and synchronizing it if so. After ensuring that the allocation bitmap is
    // consistent with the "last allocation metadata", we can finally check if
    // the txn log is consistent with the allocation bitmap, and synchronize it
    // if not. Finally, the server session thread can invoke the ordinary GC
    // code with the now-guaranteed-consistent txn log to ensure that a crashed
    // session frees all its allocations.

    struct last_allocation_metadata_t
    {
        // The previous highest allocated slot.
        slot_offset_t prev_max_allocated_slot_offset;
        // The current highest allocated slot.
        slot_offset_t max_allocated_slot_offset;
        // The lowest unallocated slot.
        // This is the slot immediately following the last allocation, and will
        // become the allocated slot for the next allocation.
        slot_offset_t min_unallocated_slot_offset;
        // Pad the struct to 8 bytes to ensure atomicity.
        uint16_t reserved;

        last_allocation_metadata_t();
    };

    static_assert(
        sizeof(last_allocation_metadata_t) == sizeof(uint64_t),
        "Expected inline_shared_lock to occupy 8 bytes!");

    static_assert(std::atomic<last_allocation_metadata_t>::is_always_lock_free);

    std::atomic<last_allocation_metadata_t> last_allocation_metadata{};

    inline void update_last_allocation_metadata(size_t allocation_size_slots)
    {
        last_allocation_metadata_t old_metadata = last_allocation_metadata;
        last_allocation_metadata_t new_metadata{};
        // TODO: Use designated initializers after C++20.
        new_metadata.prev_max_allocated_slot_offset = old_metadata.max_allocated_slot_offset;
        new_metadata.max_allocated_slot_offset = old_metadata.min_unallocated_slot_offset;
        new_metadata.min_unallocated_slot_offset = old_metadata.min_unallocated_slot_offset + allocation_size_slots;
        // Set the new "last allocation metadata" variables in a single atomic write.
        last_allocation_metadata = new_metadata;
    }

    inline slot_offset_t max_allocated_slot_offset()
    {
        return last_allocation_metadata.load().max_allocated_slot_offset;
    }

    inline slot_offset_t min_unallocated_slot_offset()
    {
        return last_allocation_metadata.load().min_unallocated_slot_offset;
    }

    inline slot_offset_t prev_max_allocated_slot_offset()
    {
        return last_allocation_metadata.load().prev_max_allocated_slot_offset;
    }

    // This bitmap records the first slot used by each allocated object. Once
    // set, a bit is never cleared; instead, a deallocation is recorded by
    // setting the corresponding bit in the deallocation bitmap. All bits are
    // set in strictly increasing order. Once a chunk is in RETIRED state, no
    // more bits can ever be set in this bitmap (until it is reused). No bits
    // can be set for slots > max_allocated_slot_offset.
    std::atomic<uint64_t> allocated_slots_bitmap[c_slot_bitmap_size];

    // These reserved variables account for how much space is remaining unused
    // out of the space we reserved for this metadata structure.
    uint64_t reserved1;
    uint64_t reserved2;

    // This bitmap records the first slot used by each deallocated object. A bit
    // can only be set in this bitmap if it was first set in the allocation
    // bitmap. Once set, a bit is never cleared. Bits may be set in arbitrary
    // order, in either the IN_USE or RETIRED chunk states.
    std::atomic<uint64_t> deallocated_slots_bitmap[c_slot_bitmap_size];

    inline slot_offset_t get_last_allocated_slot_from_bitmap();

    // We need to call this method whenever the "last allocation metadata" is
    // possibly inconsistent with the allocation bitmap, which should only
    // happen when the client session thread crashes.
    inline void synchronize_allocation_metadata()
    {
        // Find the last set bit in the allocation bitmap.
        slot_offset_t last_allocated_slot_from_bitmap = get_last_allocated_slot_from_bitmap();

        // Check to be sure the last set bit points to the same slot as
        // max_allocated_slot_offset. If not, then it must point to
        // prev_max_allocated_slot_offset (since metadata is updated
        // atomically).
        if (last_allocated_slot_from_bitmap != max_allocated_slot_offset())
        {
            ASSERT_INVARIANT(
                last_allocated_slot_from_bitmap == prev_max_allocated_slot_offset(),
                "Last allocated slot in bitmap must point to either last allocation or previous allocation!");

            // Fix up the allocation bitmap.
            set_bit_value(
                allocated_slots_bitmap, c_slot_bitmap_size,
                max_allocated_slot_offset(), true);
        }
    }

    inline void clear()
    {
        shared_lock.clear();
        last_allocation_metadata.store({});
        std::fill(allocated_slots_bitmap, allocated_slots_bitmap + c_slot_bitmap_size, 0);
        std::fill(deallocated_slots_bitmap, deallocated_slots_bitmap + c_slot_bitmap_size, 0);
    }
};

// This assert could be edited if we need to use more memory for the metadata.
static_assert(
    sizeof(chunk_manager_metadata_t) == sizeof(uint64_t) * 1024 * 2,
    "chunk_manager_metadata_t is expected to be 16kB!");

// This assert should never need to be changed, unless the design is radically updated.
// This is because a slot is the smallest allocation unit of a chunk manager,
// so it does not make sense for metadata to only use a slot partially - we'll just
// reserve any remaining space in a slot for future use.
static_assert(
    sizeof(chunk_manager_metadata_t) % c_slot_size_bytes == 0,
    "The size of chunk_manager_metadata_t is expected to be a multiple of the slot size!");

// Constants for the range of available slots within a chunk.
constexpr slot_offset_t c_first_slot_offset = sizeof(chunk_manager_metadata_t) / c_slot_size_bytes;
constexpr slot_offset_t c_last_slot_offset = -1;

constexpr size_t c_data_pages_in_chunk_count{
    (c_chunk_size_bytes - sizeof(chunk_manager_metadata_t)) / c_page_size_bytes};

constexpr size_t c_data_pages_size_bytes = c_data_pages_in_chunk_count * c_page_size_bytes;

// Defining all these functions here because they depend on c_first_slot_offset,
// which isn't available until this point (due to its dependence on sizeof).

static slot_offset_t bit_index_to_slot(size_t bit_index)
{
    return static_cast<slot_offset_t>(bit_index + c_first_slot_offset);
}

inline slot_offset_t chunk_manager_metadata_t::get_last_allocated_slot_from_bitmap()
{
    size_t bit_index = find_last_set_bit(allocated_slots_bitmap, c_slot_bitmap_size);
    if (bit_index == c_max_bit_index)
    {
        return c_invalid_slot_offset;
    }
    return bit_index_to_slot(bit_index);
}

inline chunk_manager_metadata_t::last_allocation_metadata_t::last_allocation_metadata_t()
    : prev_max_allocated_slot_offset(c_invalid_slot_offset), max_allocated_slot_offset(c_invalid_slot_offset), min_unallocated_slot_offset(c_first_slot_offset), reserved(0)
{
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
