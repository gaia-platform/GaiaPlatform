/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <algorithm>

#include "access_control.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

// A chunk manager's metadata information.
struct chunk_manager_metadata_t
{
    // A 4MB chunk can store 2^16 64B slots.
    // A bitmap for 2^16 slots takes 8kB, or the space of 128 slots.
    // Because the bitmap does not need to track those 128 slots, that frees 16B.
    // In terms of 8B words, the bitmap only needs 1024 - 2 such words for its tracking.
    // The 2 words can be used to store additional metadata.
    static constexpr address_offset_t c_slot_bitmap_size = 1024 - 2;

    slot_offset_t last_committed_slot_offset;
    slot_offset_t last_allocated_slot_offset;
    uint32_t reserved1;
    uint64_t reserved2;
    uint64_t slot_bitmap[c_slot_bitmap_size];

    inline void clear()
    {
        last_committed_slot_offset = c_invalid_slot_offset;
        last_allocated_slot_offset = c_invalid_slot_offset;
        std::fill(slot_bitmap, slot_bitmap + c_slot_bitmap_size, 0);
    }
};

static_assert(
    sizeof(chunk_manager_metadata_t) == 8 * 1024,
    "chunk_manager_metadata_t was expected to be 8kB!");

} // namespace memory_manager
} // namespace db
} // namespace gaia
