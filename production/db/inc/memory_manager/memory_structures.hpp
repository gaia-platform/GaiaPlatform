/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <atomic>

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
    // Our "allocated chunk bitmap" only stores a boolean per chunk,
    // so we need 1 bit/chunk.
    // Therefore our bitmap contains 2^16 bits = 8KB, or 1024 64-bit words.
    static constexpr size_t c_chunk_bitmap_size_in_words{
        (1UL << (CHAR_BIT * sizeof(chunk_offset_t))) / common::c_uint64_bit_count};

    // This is a physical array of 64-bit words, storing a logical array of
    // bits, each of which denotes whether the chunk at its logical index is
    // available for allocation. The "available" status of each chunk recorded
    // in this bitmap should be considered only a "hint": the "source of truth"
    // is the chunk state within each chunk's inline metadata. Because this
    // bitmap is updated non-atomically with the chunk state (it is always
    // updated after the state), it may indicate that a chunk is available when
    // it is not, or vice versa. Even if the chunk state is consistent with the
    // bitmap when we check it, it may no longer be when we attempt to
    // transition the chunk state. If the bitmap indicates that a chunk is
    // available, but it is no longer available when we attempt the transition
    // from EMPTY to IN_USE, the transition will fail and we will try the next
    // chunk marked "available" in the bitmap. If the bitmap indicates that a
    // chunk is not available when it is available, then in the worst case we
    // will allocate an unused chunk unnecessarily. In that case, another
    // session will eventually allocate the empty chunk we overlooked, so this
    // should not be a problem in practice.
    std::atomic<uint64_t> allocated_chunks_bitmap[c_chunk_bitmap_size_in_words];

    // As we keep allocating memory, the available contiguous virtual memory
    // region will keep shrinking. We'll use this offset to track the
    // lowest-numbered chunk that has never been allocated.
    // NB: We use size_t here rather than chunk_offset_t to avoid integer
    // overflow. A 64-bit atomically incremented counter cannot overflow in any
    // reasonable time.
    std::atomic<size_t> next_available_unused_chunk_offset{};

    // These reserved variables account for how much space is remaining unused
    // out of the space we reserved for this metadata structure.
    // The reserved array covers the bulk of the space as a multiple of 8B words
    // and any remainder smaller than 8B is covered by individual variables.
    uint64_t reserved[c_chunk_size_in_bytes / sizeof(uint64_t) - c_chunk_bitmap_size_in_words - (sizeof(next_available_unused_chunk_offset) / sizeof(uint64_t))];

    inline memory_manager_metadata_t();

    inline void clear();
};

// Constants for the range of available chunks within our memory.
constexpr chunk_offset_t c_first_chunk_offset = sizeof(memory_manager_metadata_t) / c_chunk_size_in_bytes;
constexpr chunk_offset_t c_last_chunk_offset = -1;

// This assert should be edited if we need to use more memory for the metadata.
static_assert(
    sizeof(memory_manager_metadata_t) == c_chunk_size_in_bytes,
    "The size of memory_manager_metadata_t is expected to be 4MB!");

// This assert should never need to be changed, unless the design is radically updated.
// This is because a chunk is the smallest allocation unit of a memory manager,
// so it does not make sense for metadata to only use a chunk partially - we'll just
// reserve any remaining space in a chunk for future use.
static_assert(
    sizeof(memory_manager_metadata_t) % c_chunk_size_in_bytes == 0,
    "The size of memory_manager_metadata_t is expected to be a multiple of the chunk size!");

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

// REVIEW: I think we could eliminate this extra metadata and its non-atomic
// update WRT the allocation bitmap, if we tracked the final slot (+1) of
// each allocation in the bitmaps, instead of the initial slot. That would
// simplify the consistency invariants (because only the bitmap would be
// updated on allocation), but would complicate conversion of an allocation
// bitmap index to a gaia_offset or vice versa (to find the gaia_offset
// corresponding to a set bit in the allocation bitmap, we would need to
// scan backward to find the preceding set bit and then convert its index to
// a gaia_offset).

struct last_allocation_metadata_t
{
    // The previous highest allocated slot.
    slot_offset_t prev_max_allocated_slot_offset;

    // The current highest allocated slot.
    slot_offset_t max_allocated_slot_offset;

    // The lowest unallocated slot.
    // This is the slot immediately following the slot at the end of the last
    // allocation, and will become the allocated slot for the next allocation.
    slot_offset_t min_unallocated_slot_offset;

    // Pad the struct to 8 bytes to ensure atomicity.
    uint16_t reserved;

    inline last_allocation_metadata_t();
};

static_assert(
    sizeof(last_allocation_metadata_t) == sizeof(uint64_t),
    "Expected last_allocation_metadata_t to occupy 8 bytes!");

static_assert(std::atomic<last_allocation_metadata_t>::is_always_lock_free);

struct valid_chunk_transition_t
{
    chunk_state_t old_state;
    chunk_state_t new_state;
};

static inline constexpr valid_chunk_transition_t c_valid_chunk_transitions[] = {
    // ALLOCATE
    {chunk_state_t::empty, chunk_state_t::in_use},
    // RETIRE
    {chunk_state_t::in_use, chunk_state_t::retired},
    // BEGIN_DEALLOCATE
    {chunk_state_t::retired, chunk_state_t::deallocating},
    // END_DEALLOCATE
    {chunk_state_t::deallocating, chunk_state_t::empty},
};

// A chunk manager's metadata information.
struct chunk_manager_metadata_t
{
    // A 4MB chunk can store 2^16 64B slots. A bitmap for 2^16 slots takes 8KB,
    // or the space of 128 slots. We have 2 such bitmaps, so together they
    // occupy the space of 256 slots. Because the bitmaps do not need to track
    // those 256 slots, that frees 32B per bitmap (64B combined), so each bitmap
    // needs 2^10 - 4 64-bit words.
    static constexpr size_t c_bitmap_count{2};
    static constexpr size_t c_total_slots_count{
        (1UL << (CHAR_BIT * sizeof(slot_offset_t)))};
    static constexpr size_t c_slot_bitmap_size_in_words_raw{
        c_total_slots_count / common::c_uint64_bit_count};
    static constexpr size_t c_slots_occupied_by_bitmap{
        (c_slot_bitmap_size_in_words_raw * sizeof(uint64_t)) / c_slot_size_in_bytes};
    static constexpr size_t c_adjusted_slots_count{
        c_total_slots_count - (c_bitmap_count * c_slots_occupied_by_bitmap)};
    static constexpr size_t c_slot_bitmap_size_in_words{
        c_adjusted_slots_count / common::c_uint64_bit_count};

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

    // This holds a 2-bit chunk state value in the low bits, plus a 62-bit
    // version counter in the high bits to avoid ABA hazards.
    // NB: 62 bits is massive overkill, especially since wraparound can be
    // tolerated if it doesn't happen while a thread is preempted (remember, the
    // version counter is only used to mitigate ABA hazards). We would probably
    // be fine with uint16_t (so 14 bits for the version counter, assuming a
    // 2-bit chunk state). If surplus bytes in the chunk header become scarce,
    // we could revisit the size of chunk_version_t.
    std::atomic<chunk_version_t::value_type> chunk_state_and_version;

    static_assert(
        sizeof(chunk_state_and_version) == sizeof(uint64_t),
        "Expected chunk_state_and_version to occupy 8 bytes!");

    // Just in case chunk_version_t is smaller than a word.
    static_assert(std::atomic<chunk_version_t::value_type>::is_always_lock_free);

    std::atomic<last_allocation_metadata_t> last_allocation_metadata{};

    // These reserved "slack" variables account for how much space is remaining
    // unused out of the space we reserved for this metadata structure. Their
    // total size in bytes should be (c_first_slot_offset / CHAR_BIT) * 2.

    // We have 32 bytes of slack for the first bitmap, and we have used 24 bytes.
    uint64_t allocated_slots_bitmap_unused_slack[1];

    // This bitmap records the first slot used by each allocated object. Once
    // set, a bit is never cleared; instead, a deallocation is recorded by
    // setting the corresponding bit in the deallocation bitmap. All bits are
    // set in strictly increasing order. Once a chunk is in RETIRED state, no
    // more bits can ever be set in this bitmap (until it is reused). No bits
    // can be set for slots > max_allocated_slot_offset.
    std::atomic<uint64_t> allocated_slots_bitmap[c_slot_bitmap_size_in_words];

    // We have 32 bytes of slack for the second bitmap, and we have used 0 bytes.
    uint64_t deallocated_slots_bitmap_unused_slack[4];

    // This bitmap records the first slot used by each deallocated object. A bit
    // can only be set in this bitmap if it was first set in the allocation
    // bitmap. Once set, a bit is never cleared. Bits may be set in arbitrary
    // order, in either the IN_USE or RETIRED chunk states.
    std::atomic<uint64_t> deallocated_slots_bitmap[c_slot_bitmap_size_in_words];

    // We need to call this method whenever the "last allocation metadata" is
    // possibly inconsistent with the allocation bitmap, which should only
    // happen when the client session thread crashes.
    inline void synchronize_allocation_metadata();

    inline std::pair<chunk_state_t, chunk_version_t>
    get_chunk_state_and_version() const;

    inline chunk_state_t get_chunk_state() const;

    inline chunk_version_t get_chunk_version() const;

    inline bool apply_chunk_transition_from_version(
        chunk_version_t expected_version, chunk_state_t expected_state, chunk_state_t desired_state);

    inline bool apply_chunk_transition(
        chunk_state_t expected_state, chunk_state_t desired_state);

    inline void update_last_allocation_metadata(size_t allocation_size_slots);

    inline slot_offset_t max_allocated_slot_offset() const;

    inline slot_offset_t min_unallocated_slot_offset() const;

    inline slot_offset_t prev_max_allocated_slot_offset() const;

    inline void clear();
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
    sizeof(chunk_manager_metadata_t) % c_slot_size_in_bytes == 0,
    "The size of chunk_manager_metadata_t is expected to be a multiple of the slot size!");

// Constants for the range of available slots within a chunk.
constexpr slot_offset_t c_first_slot_offset{
    sizeof(chunk_manager_metadata_t) / c_slot_size_in_bytes};

constexpr slot_offset_t c_last_slot_offset{
    std::numeric_limits<slot_offset_t::value_type>::max()};

constexpr size_t c_chunk_data_pages_count{
    (c_chunk_size_in_bytes - sizeof(chunk_manager_metadata_t)) / c_page_size_in_bytes};

constexpr size_t c_chunk_data_pages_size_in_bytes{
    c_chunk_data_pages_count * c_page_size_in_bytes};

#include "memory_structures.inc"

} // namespace memory_manager
} // namespace db
} // namespace gaia
