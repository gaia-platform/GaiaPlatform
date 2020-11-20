/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_id_mapping.hpp"

#include "catalog_core.hpp"
#include "gaia_common.hpp"
#include "retail_assert.hpp"

using namespace gaia::db;

gaia::common::gaia_id_t type_id_mapping_t::get_record_id(gaia::common::gaia_type_t type_id)
{
    std::shared_lock lock(m_lock);

    std::call_once(*m_is_initialized, &type_id_mapping_t::init_type_map, this);

    if (m_type_map.find(type_id) == m_type_map.end())
    {
        return c_invalid_gaia_id;
    }
    else
    {
        return m_type_map.at(type_id);
    }
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
