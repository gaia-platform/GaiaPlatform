/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "payload_diff.hpp"

#include <string>
#include <vector>

#include "auto_transaction.hpp"
#include "catalog_view.hpp"
#include "data_holder.hpp"
#include "field_access.hpp"
#include "gaia_common.hpp"
#include "retail_assert.hpp"
#include "system_table_types.hpp"
#include "type_cache.hpp"

namespace gaia
{
namespace db
{
namespace payload_types
{

// TODO: Add unit tests for this function.
void compute_payload_diff(
    gaia_id_t type_id,
    const uint8_t* payload1,
    const uint8_t* payload2,
    field_position_list_t* changed_fields)
{
    // Make sure caller passes valid pointer to changed_fields.
    retail_assert(changed_fields);

    gaia_id_t type_table_id = type_cache_t::get()->get_table_id(type_id);
    auto schema = catalog_view_t::get_table(type_table_id).binary_schema();

    for (auto field_view : catalog_view_t::list_fields(type_table_id))
    {
        if (field_view.data_type() == data_type_t::e_references)
        {
            continue;
        }
        field_position_t pos = field_view.position();
        data_holder_t data_holder1 = get_field_value(type_id, payload1, schema.data(), pos);
        data_holder_t data_holder2 = get_field_value(type_id, payload2, schema.data(), pos);

        // Compare values and set.
        if (data_holder1.compare(data_holder2) != 0)
        {
            changed_fields->push_back(pos);
        }
    }
}

} // namespace payload_types
} // namespace db
} // namespace gaia
