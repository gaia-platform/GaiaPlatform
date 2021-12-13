/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "index.hpp"

namespace gaia
{
namespace db
{
namespace index
{

bool index_offset_buffer_t::has_offset(gaia_offset_t offset) const
{
    return std::find(m_offsets.cbegin(), m_offsets.cend(), offset) != m_offsets.cend();
}

bool index_offset_buffer_t::has_type(common::gaia_type_t type) const
{
    return std::find(m_offset_types.cbegin(), m_offset_types.cend(), type) != m_offset_types.cend();
}

size_t index_offset_buffer_t::size() const
{
    return m_size;
}

bool index_offset_buffer_t::empty() const
{
    return m_offsets.empty();
}

void index_offset_buffer_t::insert(gaia_offset_t offset, common::gaia_type_t type)
{
    m_offsets[m_size] = offset;
    m_offset_types[m_size] = type;
    ++m_size;
}
} // namespace index
} // namespace db
} // namespace gaia
