/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_id_mapping.hpp"

#include "catalog_core.hpp"
#include "retail_assert.hpp"

using namespace gaia::db;

gaia::common::gaia_id_t type_id_mapping_t::get_record_id(gaia::common::gaia_type_t type_id)
{
    std::shared_lock lock(m_lock);

    std::call_once(*m_is_initialized, &type_id_mapping_t::init_type_map, this);

    auto it = m_type_map.find(type_id);

    // TODO Chuan modified this logic to return c_invalid_gaia_id in #373
    //   That should be the final version.
    if (it == m_type_map.end())
    {
        throw invalid_type(type_id);
    }

    return it->second;
}

void type_id_mapping_t::init_type_map()
{
    m_type_map.clear();

    for (auto table_view : catalog_core_t::list_tables())
    {
        m_type_map[table_view.table_type()] = table_view.id();
    }
}

void type_id_mapping_t::clear()
{
    std::unique_lock lock(m_lock);

    m_is_initialized = std::make_unique<std::once_flag>();
}
