/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "data_buffer.hpp"

#include <algorithm>

namespace gaia
{
namespace db
{
data_write_buffer_t::data_write_buffer_t(flatbuffers::FlatBufferBuilder& builder)
    : m_buffer(), m_builder(builder)
{
}

data_read_buffer_t::data_read_buffer_t(const serialization_buffer_t& buffer)
    : m_buffer_ptr(reinterpret_cast<const char*>(buffer.Data()))

{
}

const char* data_read_buffer_t::read(size_t size)
{
    const char* ptr = m_buffer_ptr;
    m_buffer_ptr += size;
    return ptr;
}

void data_write_buffer_t::write(const char* buffer, size_t size)
{
    auto ptr = reinterpret_cast<const serialization_byte_t*>(buffer);
    std::copy(ptr, ptr + size, std::back_inserter(m_buffer));
}

serialization_output_t data_write_buffer_t::output()
{
    return m_builder.CreateVector(m_buffer);
}

} // namespace db
} // namespace gaia
