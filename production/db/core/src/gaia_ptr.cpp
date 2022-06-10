////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "gaia_internal/db/gaia_ptr.hpp"

#include <cstring>

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/db/type_metadata.hpp"

#include "db_helpers.hpp"
#include "type_index.hpp"
#include "type_index_cursor.hpp"

using namespace gaia::common;
using namespace gaia::common::iterators;

namespace gaia
{
namespace db
{

gaia_id_t gaia_ptr_t::generate_id()
{
    return allocate_id();
}

gaia_ptr_t gaia_ptr_t::from_locator(
    gaia_locator_t locator)
{
    return gaia_ptr_t(locator);
}

gaia_ptr_t gaia_ptr_t::from_gaia_id(
    common::gaia_id_t id)
{
    return gaia_ptr_t(id_to_locator(id));
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
    if (m_locator.is_valid())
    {
        return find_next(to_ptr()->type);
    }

    return *this;
}

gaia_ptr_t gaia_ptr_t::find_next(gaia_type_t type) const
{
    gaia_ptr_t next_ptr = *this;

    // Search for objects of this type within the range of used locators.
    gaia_locator_t last_locator = get_last_locator();
    while ((++next_ptr.m_locator).is_valid() && next_ptr.m_locator <= last_locator)
    {
        if (next_ptr.is(type))
        {
            return next_ptr;
        }
    }

    // Mark end of search.
    next_ptr.m_locator = c_invalid_gaia_locator;
    return next_ptr;
}

std::shared_ptr<generator_t<gaia_locator_t>>
gaia_ptr_t::get_locator_generator_for_type(gaia_type_t type)
{
    type_index_t* type_index = get_type_index();
    type_index_cursor_t cursor(type_index, type);

    // Scan locator node list for this type, helping to finish any deletions in progress.
    auto locator_generator = [cursor]() mutable -> std::optional<gaia_locator_t> {
        if (!cursor)
        {
            // We've reached the end of the list, so signal end of iteration.
            return std::nullopt;
        }
        // Is current node marked for deletion? If so, unlink it (and all
        // consecutive marked nodes) from the list and continue.
        if (cursor.is_current_node_deleted())
        {
            // We ignore failures because a subsequent scan will retry.
            cursor.unlink_for_deletion();
        }
        // Save the current locator before we advance the cursor.
        auto current_locator = cursor.current_locator();
        cursor.advance();
        return current_locator;
    };

    return std::make_shared<generator_t<gaia_locator_t>>(locator_generator);
}

gaia_ptr_generator_t::gaia_ptr_generator_t(std::shared_ptr<generator_t<gaia_locator_t>> locator_generator)
    : m_locator_generator(std::move(locator_generator))
{
}

std::optional<gaia_ptr_t> gaia_ptr_generator_t::operator()()
{
    std::optional<gaia_locator_t> locator_opt;
    while ((locator_opt = (*m_locator_generator)()))
    {
        gaia_ptr_t gaia_ptr = gaia_ptr_t::from_locator(*locator_opt);
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
    return generator_iterator_t<gaia_ptr_t>(gaia_ptr_generator_t(get_locator_generator_for_type(type)));
}

generator_range_t<gaia_ptr_t> gaia_ptr_t::find_all_range(
    gaia_type_t type)
{
    return range_from_generator_iterator(find_all_iterator(type));
}

} // namespace db
} // namespace gaia
