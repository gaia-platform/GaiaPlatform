/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_caches.hpp"

#include "gaia_internal/common/retail_assert.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace caches
{

table_relationship_cache_t table_relationship_cache_t::s_cache;

table_relationship_cache_t* table_relationship_cache_t::get()
{
    return &s_cache;
}

bool table_relationship_cache_t::is_initialized()
{
    return m_is_initialized;
}

void table_relationship_cache_t::put(gaia::common::gaia_id_t table_id)
{
    ASSERT_PRECONDITION(
        m_map.find(table_id) == m_map.end(),
        "table_relationship_cache_t already contained an entry for the given table id!");

    m_map.insert(make_pair(table_id, field_position_set_t()));

    m_is_initialized = true;
}

void table_relationship_cache_t::put(gaia::common::gaia_id_t table_id, gaia::common::field_position_t field)
{
    auto iterator = m_map.find(table_id);

    ASSERT_INVARIANT(
        iterator != m_map.end(), "table_relationship_cache_t was queried for a table id that it did not contain!");

    // Do not check result.
    // The same field could be added several times, due to its involvement in multiple relationships.
    iterator->second.insert(field);
}

field_position_set_t& table_relationship_cache_t::get(gaia::common::gaia_id_t table_id)
{
    auto iterator = m_map.find(table_id);

    ASSERT_INVARIANT(
        iterator != m_map.end(), "table_relationship_cache_t was queried for a table id that it did not contain!");

    return iterator->second;
}

size_t table_relationship_cache_t::size() const
{
    return m_map.size();
}

} // namespace caches
} // namespace db
} // namespace gaia
