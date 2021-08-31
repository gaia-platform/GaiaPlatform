/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <cstring>

#include <vector>

#include "flatbuffers/flatbuffers.h"

namespace gaia
{
namespace db
{

// byte type in flatbuffers translates to int8_t.
using serialization_byte_t = int8_t;
using serialization_buffer_t = flatbuffers::Vector<serialization_byte_t>;
using serialization_write_buffer_t = std::vector<serialization_byte_t>;
using serialization_output_t = flatbuffers::Offset<flatbuffers::Vector<serialization_byte_t>>;

// These classes provide serialization/deserialization interfaces over the flatbuffers type [byte]
// in Flatbuffer messages.

// data_write_buffer_t provides a write() interface over a FlatBufferBuilder.
class data_write_buffer_t
{
public:
    // builder - The FlatBufferBuilder that holds the data needed for our final FlatBuffer object.
    explicit data_write_buffer_t(flatbuffers::FlatBufferBuilder& builder);
    data_write_buffer_t(const data_write_buffer_t& other) = delete;

    void write(const char* data, size_t size);
    serialization_output_t output();

    template <typename T_value>
    data_write_buffer_t& operator<<(T_value value);

private:
    serialization_write_buffer_t m_buffer;
    flatbuffers::FlatBufferBuilder& m_builder;
};

// data_read_buffer_t provides read() interface over the [byte] field from a FlatBuffer object.
class data_read_buffer_t
{
public:
    explicit data_read_buffer_t(const serialization_buffer_t& buffer);
    data_read_buffer_t(const data_read_buffer_t& other) = delete;

    const char* read(size_t size);

    template <typename T_value>
    data_read_buffer_t& operator>>(T_value& value);

private:
    const char* m_buffer_ptr;
};

#include "data_buffer.inc"

} // namespace db
} // namespace gaia
