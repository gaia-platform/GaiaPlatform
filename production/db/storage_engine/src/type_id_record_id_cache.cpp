/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_id_record_id_cache.hpp"

#include "catalog_core.hpp"
#include "logger.hpp"
#include "retail_assert.hpp"

using namespace gaia::db;

gaia::common::gaia_id_t type_id_record_id_cache_t::get_record_id(gaia::common::gaia_type_t type_id)
{

    if (!m_initialized)
    {
        std::unique_lock lock(m_cache_lock);
        if (!m_initialized)
        {
            init_type_id_record_id_map();
            m_initialized = true;
        }
    }

    auto it = m_type_id_record_id_map.find(type_id);

    retail_assert(it != m_type_id_record_id_map.end(), "The type " + std::to_string(type_id) + " does not exist in the catalog.");

    return it->second;
}

void type_id_record_id_cache_t::init_type_id_record_id_map()
{
    for (auto table_view : catalog_core_t::list_tables())
    {
        m_type_id_record_id_map[table_view.table_type()] = table_view.id();
    }
}

void type_id_record_id_cache_t::clear()
{
    std::unique_lock lock(m_cache_lock);
    m_initialized = false;
    m_type_id_record_id_map.clear();
}
