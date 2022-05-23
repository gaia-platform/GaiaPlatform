////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <climits>
#include <cstdint>

#include "gaia_internal/common/assert.hpp"

namespace gaia
{
namespace db
{

// This alignment applies to chunks and to individual object allocations within
// chunks.
constexpr size_t c_allocation_alignment = 8 * sizeof(uint64_t);

// Our allocation slots are currently the size of our alignment.
constexpr size_t c_slot_size_in_bytes = c_allocation_alignment;

// Our maximum allocation size is currently 64KB, or 1K slots.
constexpr size_t c_max_allocation_size_in_slots = 1024;

// Memory manager allocates memory in chunks, from which clients allocate memory
// for individual objects.
constexpr size_t c_chunk_size_in_bytes = 4 * 1024 * 1024;

// We assume everywhere that OS pages are 4KB (this is verified at server
// startup).
constexpr size_t c_page_size_in_bytes = 4096;

// For representing slot offsets within a chunk.
// The total number of 64B slots in a 4MB chunk can be represented using a
// 16-bit integer. A slot_offset_t value is just the low 16 bits of a 32-bit
// gaia_offset_t; it is the offset of an object in 64B units, relative to the
// beginning of its 4MB chunk, while a gaia_offset_t value is an object's offset
// in 64B units, relative to the base address of the data segment.
// Because chunks start with a metadata block, we use 0 to represent an invalid
// slot offset.
class slot_offset_t : public common::int_type_t<uint16_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr slot_offset_t()
        : common::int_type_t<uint16_t, 0>()
    {
    }

    constexpr slot_offset_t(uint16_t value)
        : common::int_type_t<uint16_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(slot_offset_t) == sizeof(slot_offset_t::value_type),
    "slot_offset_t has a different size than its underlying integer type!");

constexpr slot_offset_t c_invalid_slot_offset;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_slot_offset.value() == slot_offset_t::c_default_invalid_value,
    "Invalid c_invalid_slot_offset initialization!");

// For representing chunk offsets within a range of memory.
// The total number of 4MB chunks in 256GB memory can be represented using a
// 16-bit integer. A chunk_offset_t value is just the high 16 bits of a 32-bit
// gaia_offset_t; it is the offset of an object in 4MB units, relative to the
// base address of the data segment.
// Because memory starts with a metadata block, we use 0 to represent an invalid
// chunk offset.
class chunk_offset_t : public common::int_type_t<uint16_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr chunk_offset_t()
        : common::int_type_t<uint16_t, 0>()
    {
    }

    constexpr chunk_offset_t(uint16_t value)
        : common::int_type_t<uint16_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(chunk_offset_t) == sizeof(chunk_offset_t::value_type),
    "chunk_offset_t has a different size than its underlying integer type!");

constexpr chunk_offset_t c_invalid_chunk_offset;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_chunk_offset.value() == chunk_offset_t::c_default_invalid_value,
    "Invalid c_invalid_chunk_offset initialization!");

// A 62-bit sequential version counter (in the high bits) concatenated with the
// 2-bit chunk state (in the low bits).
class chunk_version_t : public common::int_type_t<uint64_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr chunk_version_t()
        : common::int_type_t<uint64_t, 0>()
    {
    }

    constexpr chunk_version_t(uint64_t value)
        : common::int_type_t<uint64_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(chunk_version_t) == sizeof(chunk_version_t::value_type),
    "chunk_version_t has a different size than its underlying integer type!");

constexpr chunk_version_t c_invalid_chunk_version;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_chunk_version.value() == chunk_version_t::c_default_invalid_value,
    "Invalid c_invalid_chunk_version initialization!");

// These states must all fit into 2 bits. The underlying type is compatible with
// the type of the chunk version counter, so they can be combined without casts.
enum class chunk_state_t : chunk_version_t::value_type
{
    empty = 0b00,
    in_use = 0b01,
    retired = 0b10,
    deallocating = 0b11,

    last_value = deallocating
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
    case chunk_state_t::deallocating:
        os << "deallocating";
        break;
    default:
        ASSERT_UNREACHABLE("Unknown value of chunk_state_t!");
    }
    return os;
}

constexpr size_t c_chunk_state_bit_width{2};
constexpr size_t c_chunk_state_shift{0};
constexpr size_t c_chunk_version_bit_width{
    (CHAR_BIT * sizeof(chunk_version_t)) - c_chunk_state_bit_width};
constexpr size_t c_chunk_version_shift{c_chunk_state_bit_width};

static_assert(
    common::get_enum_value(chunk_state_t::last_value) < (1 << c_chunk_state_bit_width));

} // namespace db
} // namespace gaia

namespace std
{

// This enables chunk_offset_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::db::chunk_offset_t>
{
    size_t operator()(const gaia::db::chunk_offset_t& chunk_offset) const noexcept
    {
        return std::hash<gaia::db::chunk_offset_t::value_type>()(chunk_offset.value());
    }
};

} // namespace std
