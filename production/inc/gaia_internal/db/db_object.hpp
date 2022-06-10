////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <iomanip>
#include <ostream>

#include "gaia/common.hpp"

#include <gaia_spdlog/fmt/fmt.h>

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
 * + 2 (references_count) = 16.
 */
struct alignas(gaia::db::c_allocation_alignment) db_object_t
{
    gaia::common::gaia_id_t id;
    gaia::common::gaia_type_t type;

    // The flatbuffer size limit is 2GB (the maximum value of a signed 32-bit
    // word). With a 16-bit payload size, the limit is 64KB. The total size of
    // the payload is the serialized flatbuffer size plus the size of the
    // references array (references_count * sizeof(gaia_id_t)).
    uint16_t payload_size;
    gaia::common::reference_offset_t references_count;

    // This contains an array of zero or more references (gaia_id_t), followed by
    // a serialized flatbuffer object.
    // Flexible array members are not standardized, but are supported by both gcc and clang.
    char payload[];

    /**
     * Returns the actual size in bytes used by the object, regardless of
     * alignment constraints. The result can be less than or greater than
     * `sizeof(db_object_t)`, because of the alignment constraint and the
     * flexible array member `payload`, respectively.
     *
     * We have to define this method outside the struct definition because it
     * references c_db_object_header_size, which depends on the struct
     * definition.
     */
    [[nodiscard]] inline size_t total_size() const;

    /**
     * Returns a pointer to the object's data payload.
     */
    [[nodiscard]] const char* data() const
    {
        return payload + references_count * sizeof(gaia::common::gaia_id_t);
    }

    /**
     * Returns the size in bytes of the object's data payload.
     */
    [[nodiscard]] size_t data_size() const
    {
        return payload_size - (references_count * sizeof(gaia::common::gaia_id_t));
    }

    /**
     * Returns a pointer to the first element of the object's references array.
     */
    [[nodiscard]] const gaia::common::gaia_id_t* references() const
    {
        return reinterpret_cast<const gaia::common::gaia_id_t*>(payload);
    }

    friend std::ostream& operator<<(std::ostream& os, const db_object_t& o)
    {
        os << "id: "
           << o.id
           << "\ttype: "
           << o.type
           << "\tpayload_size: "
           << o.payload_size
           << "\treferences_count: "
           << o.references_count
           << std::endl;

        os << "references:" << std::endl;
        for (size_t i = 0; i < o.references_count; ++i)
        {
            os << o.references()[i] << std::endl;
        }
        os << std::endl;

        size_t data_size = o.data_size();
        os << "data (hex):" << std::endl;

        for (size_t i = 0; i < data_size; ++i)
        {
            os << gaia_fmt::format("{:#04x}", static_cast<uint8_t>(o.data()[i])) << " ";
        }
        os << std::endl;

        os << "data (ASCII):" << std::endl;
        for (size_t i = 0; i < data_size; ++i)
        {
            os << o.data()[i] << " ";
        }
        os << std::endl;

        return os;
    }
};

// Because the type is explicitly aligned to a granularity larger than
// its nominal size, we cannot use sizeof() to compute the size of the
// object header!
constexpr size_t c_db_object_header_size = offsetof(db_object_t, payload);

// Due to our memory management requirements, we never want the object header
// size to be larger than the object alignment.
static_assert(c_db_object_header_size <= gaia::db::c_allocation_alignment, "Object header size must not exceed object alignment!");

// We need to 8-byte-align both the references array at the beginning of the
// payload (since references are 8 bytes) and the serialized flatbuffer that
// follows it (since 8 bytes is the largest scalar data type size supported by
// flatbuffers). Instead of forcing correct alignment via a compiler directive,
// we assert that the payload field is correctly aligned, to avoid having the
// compiler silently insert padding if the field isn't naturally aligned.
static_assert(c_db_object_header_size % sizeof(uint64_t) == 0, "Payload must be 8-byte-aligned!");

// We want to ensure that the object header size never changes accidentally.
constexpr size_t c_db_object_expected_header_size = 16;

// The object header size may change in the future, but we want to explicitly
// assert that it is a specific value to catch any inadvertent changes.
static_assert(c_db_object_header_size == c_db_object_expected_header_size, "Object header size must be 16 bytes!");

// The entire object can have maximum size 64KB, so user-defined data can
// have minimum size 0 bytes (implying that the references array and the
// serialized flatbuffer are both empty), and maximum size 64KB - 16B.
constexpr size_t c_db_object_max_size = static_cast<size_t>(std::numeric_limits<uint16_t>::max()) + 1;

constexpr size_t c_db_object_max_payload_size = c_db_object_max_size - c_db_object_header_size;

// Check for overflow.
static_assert(c_db_object_max_payload_size <= std::numeric_limits<uint16_t>::max());

// According to the standard, sizeof(T) is always a multiple of alignof(T).
// We need this multiple to be 1 to simplify memory management.
static_assert(sizeof(db_object_t) == alignof(db_object_t), "Object size must be identical to object alignment!");

[[nodiscard]] size_t db_object_t::total_size() const
{
    return c_db_object_header_size + payload_size;
}

} // namespace db
} // namespace gaia
