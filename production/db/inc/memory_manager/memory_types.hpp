/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace gaia
{
namespace db
{
namespace memory_manager
{

// For representing offsets from a base memory address.
// Because memory starts with a metadata block, we use 0 to represent an invalid address offset.
typedef uint64_t address_offset_t;
constexpr address_offset_t c_invalid_address_offset = 0;

// This alignment applies to chunks and to individual object allocations within chunks.
constexpr uint64_t c_allocation_alignment = 8 * sizeof(uint64_t);

// Our allocation slots are currently the size of our alignment.
constexpr uint64_t c_slot_size = c_allocation_alignment;

// Memory manager allocates memory in chunks, from which clients allocate memory for individual objects.
constexpr uint64_t c_chunk_size = 4 * 1024 * 1024;

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

} // namespace memory_manager
} // namespace db
} // namespace gaia
