/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/gaia_ptr.hpp"

#include <cstring>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/type_metadata.hpp"

#include "db_hash_map.hpp"
#include "db_helpers.hpp"

using namespace gaia::common;
using namespace gaia::common::iterators;

namespace gaia
{
namespace db
{

gaia_ptr_t::gaia_ptr_t(gaia_id_t id)
{
    m_locator = db_hash_map::find(id);
}

gaia_ptr_t::gaia_ptr_t(gaia_locator_t locator, bool)
{
    m_locator = locator;
}

gaia_id_t gaia_ptr_t::generate_id()
{
    return allocate_id();
}

gaia_ptr_t gaia_ptr_t::find_first(common::gaia_type_t type)
{
    gaia_ptr_t ptr;
    ptr.m_locator = c_first_gaia_locator;

    if (!ptr.is(type))
    {
        ptr = ptr.find_next(type);
    }

    return ptr;
}

gaia_ptr_t gaia_ptr_t::find_next() const
{
    if (m_locator)
    {
        return find_next(to_ptr()->type);
    }

    return *this;
}

gaia_ptr_t gaia_ptr_t::find_next(gaia_type_t type) const
{
    gaia::db::counters_t* counters = gaia::db::get_counters();
    gaia_ptr_t next_ptr = *this;

    // We need an acquire barrier before reading `last_locator`. We can
    // change this full barrier to an acquire barrier when we change to proper
    // C++ atomic types.
    __sync_synchronize();

    // Search for objects of this type within the range of used locators.
    while (++next_ptr.m_locator && next_ptr.m_locator <= counters->last_locator)
    {
        if (next_ptr.is(type))
        {
            return next_ptr;
        }
        __sync_synchronize();
    }

    // Mark end of search.
    next_ptr.m_locator = c_invalid_gaia_locator;
    return next_ptr;
}

gaia_ptr_generator_t::gaia_ptr_generator_t(std::shared_ptr<generator_t<gaia_id_t>> id_generator)
    : m_id_generator(std::move(id_generator))
{
}

std::optional<gaia_ptr_t> gaia_ptr_generator_t::operator()()
{
    std::optional<gaia_id_t> id_opt;
    while ((id_opt = (*m_id_generator)()))
    {
        gaia_ptr_t gaia_ptr = gaia_ptr_t::open(*id_opt);
        if (gaia_ptr)
        {
            return gaia_ptr;
        }
    }
    return std::nullopt;
}

generator_iterator_t<gaia_ptr_t>
gaia_ptr_t::find_all_iterator(
    gaia_type_t type)
{
    return generator_iterator_t<gaia_ptr_t>(gaia_ptr_generator_t(get_id_generator_for_type(type)));
}

generator_range_t<gaia_ptr_t> gaia_ptr_t::find_all_range(
    gaia_type_t type)
{
    return range_from_generator_iterator(find_all_iterator(type));
}

} // namespace db
} // namespace gaia
