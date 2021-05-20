/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "base_index.hpp"

gaia::common::gaia_id_t gaia::db::index::base_index_t::id() const
{
    return index_id;
}
gaia::db::index::index_type_t gaia::db::index::base_index_t::type() const
{
    return index_type;
}

std::recursive_mutex& gaia::db::index::base_index_t::get_lock() const
{
    return index_lock;
}

bool gaia::db::index::base_index_t::operator==(const base_index_t& other) const
{
    return index_id == other.index_id;
}
