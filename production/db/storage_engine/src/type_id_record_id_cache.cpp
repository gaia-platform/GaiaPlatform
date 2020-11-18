/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_id_record_id_cache.hpp"

#include "gaia/retail_assert.hpp"

#include "catalog_core.hpp"

using namespace gaia::db;

gaia::common::gaia_id_t type_id_record_id_cache_t::get_record_id(gaia::common::gaia_type_t type_id)
{
    std::call_once(m_type_id_record_id_map_init_flag, &type_id_record_id_cache_t::init_type_id_record_id_map, this);

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
