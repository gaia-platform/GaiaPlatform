/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "payload_diff.hpp"

#include <mutex>

#include "data_holder.hpp"
#include "field_access.hpp"
#include "gaia_common.hpp"
#include "light_catalog.hpp"
#include "retail_assert.hpp"

namespace gaia
{
namespace db
{

class type_id_record_id_cache_t
{
private:
    std::once_flag m_type_id_record_id_map_init_flag;
    // The map used to store ids of the gaia_table records that define the corresponding types.
    std::unordered_map<gaia_type_t, gaia_id_t> m_type_id_record_id_map;

    void init_type_table_map()
    {
        for (auto table_view : gaia::db::catalog_core_t::list_tables())
        {
            m_type_id_record_id_map[table_view.table_type()] = table_view.id();
        }
    }

public:
    // Return the id of the gaia_table record that defines a given type.
    gaia_id_t get_record_id(gaia_type_t type_id)
    {
        std::call_once(m_type_id_record_id_map_init_flag, &type_id_record_id_cache_t::init_type_table_map, this);
        return m_type_id_record_id_map.at(type_id);
    }
};

// TODO: Add unit tests for this function.
void compute_payload_diff(
    gaia_type_t type_id,
    const uint8_t* payload1,
    const uint8_t* payload2,
    field_position_list_t* changed_fields)
{
    // Make sure caller passes valid pointer to changed_fields.
    retail_assert(changed_fields);

    static type_id_record_id_cache_t type_table_cache;
    gaia_id_t type_record_id = type_table_cache.get_record_id(type_id);

    auto schema = catalog_core_t::get_table(type_record_id).binary_schema();

    for (auto field_view : catalog_core_t::list_fields(type_record_id))
    {
        if (field_view.data_type() == data_type_t::e_references)
        {
            continue;
        }

        field_position_t pos = field_view.position();
        payload_types::data_holder_t data_holder1 = payload_types::get_field_value(type_id, payload1, schema.data(), pos);
        payload_types::data_holder_t data_holder2 = payload_types::get_field_value(type_id, payload2, schema.data(), pos);

        if (data_holder1.compare(data_holder2) != 0)
        {
            changed_fields->push_back(pos);
        }
    }
}

} // namespace db
} // namespace gaia
