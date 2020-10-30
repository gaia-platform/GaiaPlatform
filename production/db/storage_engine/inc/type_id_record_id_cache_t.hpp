/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <mutex>
#include <unordered_map>

#include "gaia_common.hpp"
#include "light_catalog.hpp"

namespace gaia
{
namespace db
{

class type_id_record_id_cache_t
{
public:
    // Return the id of the gaia_table record that defines a given type.
    gaia_id_t get_record_id(gaia_type_t type_id);

private:
    std::once_flag m_type_id_record_id_map_init_flag;

    // The map used to store ids of the gaia_table records that define the corresponding types.
    std::unordered_map<gaia::common::gaia_type_t, gaia::common::gaia_id_t> m_type_id_record_id_map;

    void init_type_table_map();
};

} // namespace db
} // namespace gaia
