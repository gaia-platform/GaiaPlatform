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

gaia_offset_t index_offset_buffer_t::get_offset(size_t index) const
{
    return m_offsets[index].first;
}

common::gaia_type_t index_offset_buffer_t::get_type(size_t index) const
{
    return m_offsets[index].second;
}

size_t index_offset_buffer_t::size() const
{
    return m_size;
}

bool index_offset_buffer_t::empty() const
{
    return m_size == 0;
}

void index_offset_buffer_t::insert(gaia_offset_t offset, common::gaia_type_t type)
{
    m_offsets[m_size] = std::make_pair<gaia_offset_t, common::gaia_type_t>(std::move(offset), std::move(type));
    ++m_size;
}
} // namespace index
} // namespace db
} // namespace gaia
