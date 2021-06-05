/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "base_index.hpp"

namespace gaia
{
namespace db
{
namespace index
{

common::gaia_id_t base_index_t::id() const
{
    return m_index_id;
}
catalog::index_type_t base_index_t::type() const
{
    return m_index_type;
}

std::recursive_mutex& base_index_t::get_lock() const
{
    return m_index_lock;
}

bool base_index_t::operator==(const base_index_t& other) const
{
    return m_index_id == other.m_index_id;
}

} // namespace index
} // namespace db
} // namespace gaia
