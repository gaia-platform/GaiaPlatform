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

// This was factored out of gaia_ptr.hpp because the server needs to know
// the object format but doesn't need any gaia_ptr functionality.
struct se_object_t
{
    // Adjust this if gaia_se_object_t::payload_size ever changes size.
    static constexpr uint16_t c_max_payload_size = std::numeric_limits<uint16_t>::max();

    gaia::common::gaia_id_t id;
    gaia::common::gaia_type_t type;
    // The Flatbuffer size limit is 2GB (signed 32-bit). With a 16-bit payload size,
    // the limit is 65,536 bytes. This total size of the payload will be the
    // serialized flatbuffer size plus the num_references * sizeof(gaia_id_t).
    uint16_t payload_size;
    uint16_t num_references;
    // We need to 8-byte-align both the array of 8-byte references at the
    // beginning of the payload and the serialized flatbuffer payload that
    // follows it (since 8 bytes is the largest scalar data type size supported
    // by flatbuffers).
    alignas(8) char payload[0];

    [[nodiscard]] const char* data() const
    {
        return payload + num_references * sizeof(gaia::common::gaia_id_t);
    }

    [[nodiscard]] const gaia::common::gaia_id_t* references() const
    {
        return reinterpret_cast<const gaia::common::gaia_id_t*>(payload);
    }
};

} // namespace db
} // namespace gaia
