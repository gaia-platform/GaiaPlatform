/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "gaia_common.hpp"

namespace gaia
{
namespace db
{

class type_id_mapping_t
{
public:
    type_id_mapping_t(const type_id_mapping_t&) = delete;
    type_id_mapping_t& operator=(const type_id_mapping_t&) = delete;
    type_id_mapping_t(type_id_mapping_t&&) = delete;
    type_id_mapping_t& operator=(type_id_mapping_t&&) = delete;

    static type_id_mapping_t& instance()
    {
        static type_id_mapping_t type_id_mapping;
        return type_id_mapping;
    }

    // Return the id of the gaia_table record that defines a given type.
    common::gaia_id_t get_record_id(common::gaia_type_t type_id);

    // Clear the cache content, used for tests.
    void clear();

private:
    type_id_mapping_t()
        : m_is_initialized(std::make_unique<std::once_flag>()){};

    std::unique_ptr<std::once_flag> m_is_initialized;
    std::shared_mutex m_lock;

    // Mapping between ids of the gaia_table records and the corresponding type ID.
    std::unordered_map<common::gaia_type_t, common::gaia_id_t> m_type_map;

    void init_type_map();
};

} // namespace db
} // namespace gaia
