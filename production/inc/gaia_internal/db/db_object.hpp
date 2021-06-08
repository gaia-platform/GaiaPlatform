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
 * For convenience in memory management, objects are always aligned to 64B.
 * The entire object (including both object metadata and user-defined data)
 * may have minimum size 16B and maximum size 64KB.
 * The object metadata occupies 16 bytes: 8 (id) + 4 (type) + 2 (payload_size)
 * + 2 (num_references) = 16.
 */
struct alignas(gaia::db::memory_manager::c_allocation_alignment) db_object_t
{
    // Because the type is explicitly aligned to a granularity larger than
    // its nominal size, we cannot use sizeof() to compute the size of the
    // object header! (We use an explicit value rather than offsetof(payload)
    // so that we can assert that our expected header size is correct.)
    static constexpr size_t c_object_header_size = 16;

    // The entire object can have maximum size 64KB, so user-defined data can
    // have minimum size 0 bytes (implying that the references array and the
    // serialized flatbuffer are both empty), and maximum size 64KB - 16B.
    static constexpr uint16_t c_max_payload_size = std::numeric_limits<uint16_t>::max() - c_object_header_size + 1;

    gaia::common::gaia_id_t id;
    gaia::common::gaia_type_t type;

    // The flatbuffer size limit is 2GB (the maximum value of a signed 32-bit
    // word). With a 16-bit payload size, the limit is 64KB. The total size of
    // the payload is the serialized flatbuffer size plus the size of the
    // references array (num_references * sizeof(gaia_id_t)).
    uint16_t payload_size;
    uint16_t num_references;

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

// We need to ensure that the object header size is exactly what we
// calculated above and not additionally padded.
static_assert(offsetof(db_object_t, payload) == db_object_t::c_object_header_size);

// We need to 8-byte-align both the references array at the beginning of the
// payload (since references are 8 bytes) and the serialized flatbuffer that
// follows it (since 8 bytes is the largest scalar data type size supported by
// flatbuffers). Instead of forcing correct alignment via a compiler directive,
// we assert that the payload field is correctly aligned, to avoid having the
// compiler silently insert padding if the field isn't naturally aligned.
static_assert(offsetof(db_object_t, payload) % 8 == 0, "Payload must be 8-byte-aligned!");

// According to the standard, sizeof(T) is always a multiple of alignof(T).
// We need this multiple to be 1 to simplify memory management.
static_assert(sizeof(db_object_t) == alignof(db_object_t), "Size must be identical to alignment!");

} // namespace db
} // namespace gaia
