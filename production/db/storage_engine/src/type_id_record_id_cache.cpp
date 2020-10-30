/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_id_record_id_cache.hpp"

#include "catalog_core.hpp"
#include "gaia_db.hpp"

gaia::db::type_id_record_id_cache_t::type_id_record_id_cache_t()
{
    init_type_id_record_id_map();
}

gaia::common::gaia_id_t gaia::db::type_id_record_id_cache_t::get_record_id(gaia::common::gaia_type_t type_id)
{
    auto it = m_type_id_record_id_map.find(type_id);

    if (it == m_type_id_record_id_map.end())
    {
        throw invalid_type(type_id);
    }

    return it->second;
}

void gaia::db::type_id_record_id_cache_t::init_type_id_record_id_map()
{
    for (auto table_view : gaia::db::catalog_core_t::list_tables())
    {
        m_type_id_record_id_map[table_view.table_type()] = table_view.id();
    }
}
