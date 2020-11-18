/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <mutex>
#include <unordered_map>

#include "gaia/gaia_common.hpp"

namespace gaia
{
namespace db
{

class type_id_record_id_cache_t
{
public:
    type_id_record_id_cache_t(const type_id_record_id_cache_t&) = delete;
    type_id_record_id_cache_t& operator=(const type_id_record_id_cache_t&) = delete;
    type_id_record_id_cache_t(type_id_record_id_cache_t&&) = delete;
    type_id_record_id_cache_t& operator=(type_id_record_id_cache_t&&) = delete;

    static type_id_record_id_cache_t& instance()
    {
        static type_id_record_id_cache_t type_id_record_id_cache;
        return type_id_record_id_cache;
    }

    // Return the id of the gaia_table record that defines a given type.
    common::gaia_id_t get_record_id(common::gaia_type_t type_id);

private:
    type_id_record_id_cache_t() = default;

    std::once_flag m_type_id_record_id_map_init_flag;

    // The map used to store ids of the gaia_table records that define the corresponding types.
    std::unordered_map<common::gaia_type_t, common::gaia_id_t> m_type_id_record_id_map;

    void init_type_id_record_id_map();
};

} // namespace db
} // namespace gaia
