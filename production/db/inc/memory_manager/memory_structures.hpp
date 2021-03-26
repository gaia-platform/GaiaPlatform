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

// A memory manager's metadata information.
struct memory_manager_metadata_t
{
    // A 256GB memory space can store 2^16 4MB chunks.
    // We'll reserve a full chunk for metadata,
    // because chunks are the smallest allocation units at memory manager level.
    // A bitmap for 2^16 slots takes 8kB, or 1024 64bit values.
    static constexpr address_offset_t c_chunk_bitmap_size = 1024;

    uint64_t chunk_bitmap[c_chunk_bitmap_size];

    // As we keep allocating memory, the remaining contiguous available memory block
    // will keep shrinking. We'll use this offset to track the start of the block.
    address_offset_t start_unused_memory_offset;

    uint64_t reserved[c_chunk_size / sizeof(uint64_t) - c_chunk_bitmap_size - 1];

    inline void clear()
    {
        start_unused_memory_offset = c_invalid_slot_offset;
        std::fill(chunk_bitmap, chunk_bitmap + c_chunk_bitmap_size, 0);
    }
};

static_assert(
    sizeof(memory_manager_metadata_t) == c_chunk_size,
    "memory_manager_metadata_t is expected to be 4MB!");

// Constants for the range of available chunks within our memory.
constexpr slot_offset_t c_first_chunk_offset = sizeof(memory_manager_metadata_t) / c_chunk_size;
constexpr slot_offset_t c_last_chunk_offset = -1;

// A chunk manager's metadata information.
struct chunk_manager_metadata_t
{
    // A 4MB chunk can store 2^16 64B slots.
    // A bitmap for 2^16 slots takes 8kB, or the space of 128 slots.
    // Because the bitmap does not need to track those 128 slots, that frees 16B.
    // In terms of 64b words, the bitmap only needs 1024 - 2 such words for its tracking.
    // The 2 words can be used to store additional metadata.
    static constexpr address_offset_t c_slot_bitmap_size = 1024 - 2;

    uint64_t slot_bitmap[c_slot_bitmap_size];

    slot_offset_t last_committed_slot_offset;

    uint16_t reserved1;
    uint32_t reserved2;
    uint64_t reserved3;

    inline void clear()
    {
        last_committed_slot_offset = c_invalid_slot_offset;
        std::fill(slot_bitmap, slot_bitmap + c_slot_bitmap_size, 0);
    }
};

static_assert(
    sizeof(chunk_manager_metadata_t) == sizeof(uint64_t) * 1024,
    "chunk_manager_metadata_t is expected to be 8kB!");

// Constants for the range of available slots within a chunk.
constexpr slot_offset_t c_first_slot_offset = sizeof(chunk_manager_metadata_t) / c_slot_size;
constexpr slot_offset_t c_last_slot_offset = -1;

} // namespace memory_manager
} // namespace db
} // namespace gaia
