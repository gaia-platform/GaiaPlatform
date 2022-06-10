////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "db_caches.hpp"

#include "gaia_internal/common/assert.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace caches
{

static constexpr char c_cannot_find_table_id_in_table_relationship_fields_cache_t[]
    = "Cannot find table id in table_relationship_fields_cache_t!";

bool table_relationship_fields_cache_t::is_initialized()
{
    return m_is_initialized;
}

void table_relationship_fields_cache_t::clear()
{
    m_map.clear();
    m_is_initialized = false;
}

void table_relationship_fields_cache_t::put(
    gaia::common::gaia_id_t table_id)
{
    ASSERT_PRECONDITION(
        m_map.find(table_id) == m_map.end(),
        "table_relationship_fields_cache_t already contained an entry for the given table id!");

    m_map.insert(make_pair(table_id, make_pair(field_position_set_t(), field_position_set_t())));

    m_is_initialized = true;
}

void table_relationship_fields_cache_t::put_parent_relationship_field(
    gaia::common::gaia_id_t table_id, gaia::common::field_position_t field)
{
    auto iterator = m_map.find(table_id);

    ASSERT_INVARIANT(
        iterator != m_map.end(), c_cannot_find_table_id_in_table_relationship_fields_cache_t);

    // Do not check result.
    // The same field could be added several times, due to its involvement in multiple relationships.
    iterator->second.first.insert(field);
}

void table_relationship_fields_cache_t::put_child_relationship_field(
    gaia::common::gaia_id_t table_id, gaia::common::field_position_t field)
{
    auto iterator = m_map.find(table_id);

    ASSERT_INVARIANT(
        iterator != m_map.end(), c_cannot_find_table_id_in_table_relationship_fields_cache_t);

    // Do not check result.
    // The same field could be added several times, due to its involvement in multiple relationships.
    iterator->second.second.insert(field);
}

const pair<field_position_set_t, field_position_set_t>& table_relationship_fields_cache_t::get(
    gaia::common::gaia_id_t table_id) const
{
    auto iterator = m_map.find(table_id);

    ASSERT_INVARIANT(
        iterator != m_map.end(), c_cannot_find_table_id_in_table_relationship_fields_cache_t);

    return iterator->second;
}

size_t table_relationship_fields_cache_t::size() const
{
    return m_map.size();
}

} // namespace caches
} // namespace db
} // namespace gaia
