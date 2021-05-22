/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "base_index.hpp"

gaia::common::gaia_id_t gaia::db::index::base_index_t::id() const
{
    return m_index_id;
}
gaia::db::index::index_type_t gaia::db::index::base_index_t::type() const
{
    return m_index_type;
}

std::recursive_mutex& gaia::db::index::base_index_t::get_lock() const
{
    return m_index_lock;
}

bool gaia::db::index::base_index_t::operator==(const base_index_t& other) const
{
    return m_index_id == other.m_index_id;
}
