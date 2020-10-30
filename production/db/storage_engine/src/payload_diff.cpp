/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "payload_diff.hpp"

#include "data_holder.hpp"
#include "field_access.hpp"
#include "gaia_common.hpp"
#include "light_catalog.hpp"
#include "retail_assert.hpp"
#include "type_id_record_id_cache_t.hpp"

namespace gaia
{
namespace db
{

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
