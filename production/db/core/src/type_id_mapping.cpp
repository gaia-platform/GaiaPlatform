////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "type_id_mapping.hpp"

#include "gaia/common.hpp"

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/db/catalog_core.hpp"

using namespace gaia::db;

gaia::common::gaia_id_t type_id_mapping_t::get_table_id(gaia::common::gaia_type_t type_id)
{
    std::shared_lock lock(m_lock);

    // TODO there is a bug in GNU libstdc that will make this code hang if the init_type_map() function
    //  throws an exception and this code is called another time.
    //  https://stackoverflow.com/questions/41717579/stdcall-once-hangs-on-second-call-after-callable-threw-on-first-call
    std::call_once(*m_is_initialized, &type_id_mapping_t::init_type_map, this);

    if (m_type_map.find(type_id) == m_type_map.end())
    {
        return common::c_invalid_gaia_id;
    }
    else
    {
        return m_type_map.at(type_id);
    }
}

void type_id_mapping_t::init_type_map()
{
    m_type_map.clear();

    for (const auto& table_view : catalog_core::list_tables())
    {
        m_type_map[table_view.table_type()] = table_view.id();
    }
}

void type_id_mapping_t::clear()
{
    std::unique_lock lock(m_lock);

    m_is_initialized = std::make_unique<std::once_flag>();
}
