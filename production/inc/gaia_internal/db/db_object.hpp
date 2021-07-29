/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include "gaia/common.hpp"

#include "memory_types.hpp"

namespace gaia
{
namespace db
{

/**
 * The type of a Gaia object, containing both object metadata (gaia_id,
 * gaia_type) and user-defined data (references and flatbuffer payload).
 *
 * For convenience in memory management, objects are always aligned to 64B.
 * The entire object (including both object metadata and user-defined data)
 * may have minimum size 16B and maximum size 64KB.
 *
 * The object metadata occupies 16 bytes: 8 (id) + 4 (type) + 2 (payload_size)
 * + 2 (num_references) = 16.
 */
struct alignas(gaia::db::memory_manager::c_allocation_alignment) db_object_t
{
    gaia::common::gaia_id_t id;
    gaia::common::gaia_type_t type;

    // The flatbuffer size limit is 2GB (the maximum value of a signed 32-bit
    // word). With a 16-bit payload size, the limit is 64KB. The total size of
    // the payload is the serialized flatbuffer size plus the size of the
    // references array (num_references * sizeof(gaia_id_t)).
    uint16_t payload_size;
    gaia::common::reference_offset_t num_references;

    // This contains an array of zero or more references (gaia_id_t), followed by
    // a serialized flatbuffer object.
    char payload[];

    [[nodiscard]] const char* data() const
    {
        return payload + num_references * sizeof(gaia::common::gaia_id_t);
    }

    [[nodiscard]] const gaia::common::gaia_id_t* references() const
    {
        return reinterpret_cast<const gaia::common::gaia_id_t*>(payload);
    }
};

// Because the type is explicitly aligned to a granularity larger than
// its nominal size, we cannot use sizeof() to compute the size of the
// object header!
constexpr size_t c_db_object_header_size = offsetof(db_object_t, payload);

// We want to ensure that the object header size never changes accidentally.
constexpr size_t c_db_object_expected_header_size = 16;

// The entire object can have maximum size 64KB, so user-defined data can
// have minimum size 0 bytes (implying that the references array and the
// serialized flatbuffer are both empty), and maximum size 64KB - 16B.
constexpr size_t c_db_object_max_size = static_cast<size_t>(std::numeric_limits<uint16_t>::max()) + 1;

constexpr size_t c_db_object_max_payload_size = c_db_object_max_size - c_db_object_header_size;

// Check for overflow.
static_assert(c_db_object_max_payload_size <= std::numeric_limits<uint16_t>::max());

// The object header size may change in the future, but we want to explicitly
// assert that it is a specific value to catch any inadvertent changes.
static_assert(c_db_object_header_size == c_db_object_expected_header_size, "Object header size must be 16 bytes!");

// Due to our memory management requirements, we never want the object header
// size to be larger than the object alignment.
static_assert(c_db_object_header_size <= gaia::db::memory_manager::c_allocation_alignment, "Object header size must not exceed object alignment!");

// We need to 8-byte-align both the references array at the beginning of the
// payload (since references are 8 bytes) and the serialized flatbuffer that
// follows it (since 8 bytes is the largest scalar data type size supported by
// flatbuffers). Instead of forcing correct alignment via a compiler directive,
// we assert that the payload field is correctly aligned, to avoid having the
// compiler silently insert padding if the field isn't naturally aligned.
static_assert(c_db_object_header_size % sizeof(uint64_t) == 0, "Payload must be 8-byte-aligned!");

// According to the standard, sizeof(T) is always a multiple of alignof(T).
// We need this multiple to be 1 to simplify memory management.
static_assert(sizeof(db_object_t) == alignof(db_object_t), "Object size must be identical to object alignment!");

} // namespace db
} // namespace gaia
