/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include "gaia/common.hpp"

namespace gaia
{
namespace db
{

struct db_object_t
{
    // Adjust this if db_object_t::payload_size ever changes size.
    static constexpr uint16_t c_max_payload_size = std::numeric_limits<uint16_t>::max();

    gaia::common::gaia_id_t id;
    gaia::common::gaia_type_t type;
    // The flatbuffer size limit is 2GB (the maximum value of a signed 32-bit word). With a 16-bit payload size, the
    // limit is 64KB. The total size of the payload is the serialized flatbuffer size plus the size of the references
    // array (num_references * sizeof(gaia_id_t)).
    uint16_t payload_size;
    uint16_t num_references;
    // We need to 8-byte-align both the references array at the beginning of the payload (since references are 8 bytes)
    // and the serialized flatbuffer that follows it (since 8 bytes is the largest scalar data type size supported by
    // flatbuffers). Instead of forcing correct alignment via a compiler directive, we assert that the payload field is
    // correctly aligned, to avoid having the compiler silently insert padding if the field isn't naturally aligned.
    char payload[0];

    [[nodiscard]] const char* data() const
    {
        return payload + num_references * sizeof(gaia::common::gaia_id_t);
    }

    [[nodiscard]] const gaia::common::gaia_id_t* references() const
    {
        return reinterpret_cast<const gaia::common::gaia_id_t*>(payload);
    }
};

static_assert(offsetof(db_object_t, payload) % 8 == 0, "Payload must be 8-byte-aligned!");

} // namespace db
} // namespace gaia
