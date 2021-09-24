////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "data_buffer.hpp"

#include <algorithm>

namespace gaia
{
namespace db
{
namespace payload_types
{

data_write_buffer_t::data_write_buffer_t(flatbuffers::FlatBufferBuilder& builder)
    : m_buffer(), m_builder(builder)
{
}

data_read_buffer_t::data_read_buffer_t(const serialization_buffer_t& buffer)
    : m_buffer_ptr(reinterpret_cast<const char*>(buffer.Data()))
{
}

data_read_buffer_t::data_read_buffer_t(const char* ptr)
    : m_buffer_ptr(ptr)
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
    auto result = m_builder.CreateVector(m_buffer);
    m_buffer.clear();
    return result;
}

} // namespace payload_types
} // namespace db
} // namespace gaia
