/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include "gaia_internal/common/enum_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

// For representing offsets from a base memory address.
// Because memory starts with a metadata block, we use 0 to represent an invalid address offset.
typedef size_t address_offset_t;
constexpr address_offset_t c_invalid_address_offset = 0;

// This alignment applies to chunks and to individual object allocations within chunks.
constexpr size_t c_allocation_alignment = 8 * sizeof(uint64_t);

// Our allocation slots are currently the size of our alignment.
constexpr size_t c_slot_size_bytes = c_allocation_alignment;

// Our maximum allocation size is currently 64KB, or 1K slots.
constexpr size_t c_max_allocation_slots_size = 1024;

// Memory manager allocates memory in chunks, from which clients allocate memory for individual objects.
constexpr size_t c_chunk_size_bytes = 4 * 1024 * 1024;

// We assume everywhere that OS pages are 4KB.
constexpr size_t c_page_size_bytes = 4096;

// For representing slot offsets within a chunk.
// The total number of 64B slots in a 4MB chunk can be represented using a 16bit integer.
// Because chunks start with a metadata block, we use 0 to represent an invalid slot offset.
typedef uint16_t slot_offset_t;
constexpr slot_offset_t c_invalid_slot_offset = 0;

// For representing chunk offsets within a range of memory.
// The total number of 4MB chunks in 256GB memory can be represented using a 16bit integer.
// Because memory starts with a metadata block, we use 0 to represent an invalid chunk offset.
typedef uint16_t chunk_offset_t;
constexpr chunk_offset_t c_invalid_chunk_offset = 0;

// These states must all fit into 2 bits.
enum class chunk_state_t : uint64_t
{
    empty = 0b00,
    in_use = 0b01,
    retired = 0b10,
    pending_compaction = 0b11
};

inline std::ostream& operator<<(std::ostream& os, const chunk_state_t& o)
{
    switch (o)
    {
    case chunk_state_t::empty:
        os << "empty";
        break;
    case chunk_state_t::in_use:
        os << "in_use";
        break;
    case chunk_state_t::retired:
        os << "retired";
        break;
    case chunk_state_t::pending_compaction:
        os << "pending_compaction";
        break;
    default:
        ASSERT_UNREACHABLE("Unknown value of chunk_state_t!");
    }
    return os;
}

constexpr size_t c_chunk_state_bitarray_width = 2;

} // namespace memory_manager
} // namespace db
} // namespace gaia
