/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <cerrno>
#include <cstdlib>

#include <limits>
#include <vector>

#include "retail_assert.hpp"

namespace gaia
{
namespace common
{

/**
 * Decode the hex string encoded by "flatbuffers::BufferToHexText" to the
 * original binary data.
 *
 * @param hex_buf_c_str valid null terminated hex buffer string
 * @return buffer
 */
inline std::vector<uint8_t> flatbuffers_hex_to_buffer(const char* hex_buf_c_str)
{
    std::vector<uint8_t> binary_schema;
    const char* ptr = hex_buf_c_str;
    // The delimitation character used by the "flatbuffers::BufferToHexText" hex
    // encoding method. It is hard coded in the flatbuffers implementation.
    constexpr char c_hex_text_delim = ',';
    while (*ptr != '\0')
    {
        if (*ptr == '\n')
        {
            ptr++;
            continue;
        }
        else if (*ptr == c_hex_text_delim)
        {
            ptr++;
            continue;
        }
        else
        {
            char* endptr;
            unsigned long byte = std::strtoul(ptr, &endptr, 0);
            retail_assert(
                endptr != ptr
                    && errno != ERANGE
                    && byte <= std::numeric_limits<uint8_t>::max(),
                "Invalid hex binary schema!");
            binary_schema.push_back(static_cast<uint8_t>(byte));
            ptr = endptr;
        }
    }
    return binary_schema;
}

} // namespace common
} // namespace gaia
