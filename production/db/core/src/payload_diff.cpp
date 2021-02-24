/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "payload_diff.hpp"

#include "gaia/common.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/catalog_core.hpp"

#include "data_holder.hpp"
#include "field_access.hpp"
#include "type_id_mapping.hpp"

using namespace gaia::common;

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
    retail_assert(changed_fields, "compute_payload_diff() was called with an unexpected null 'changed_fields' argument!");

    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(type_id);

    // We have entered payload diff for the update. The data have been updated,
    // and we cannot find the type in catalog. This means we have some serious
    // data corruption bug(s).
    retail_assert(
        type_record_id != c_invalid_gaia_id,
        "The type '" + std::to_string(type_id) + "' does not exist in the catalog for payload diff!");

    auto schema = catalog_core_t::get_table(type_record_id).binary_schema();

    for (auto field_view : catalog_core_t::list_fields(type_record_id))
    {
        field_position_t field_position = field_view.position();

        if (!payload_types::are_field_values_equal(
                type_id, payload1, payload2, schema.data(), schema.size(), field_position))
        {
            changed_fields->push_back(field_position);
        }
    }
}

} // namespace db
} // namespace gaia
