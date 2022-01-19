/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "payload_diff.hpp"

#include "gaia/common.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/catalog_core.hpp"

#include <gaia_spdlog/fmt/fmt.h>

#include "field_access.hpp"
#include "type_id_mapping.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{

// TODO: Add unit tests for this function.
field_position_list_t compute_payload_diff(
    gaia_type_t type_id,
    const uint8_t* payload1,
    const uint8_t* payload2)
{
    field_position_list_t changed_fields;

    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(type_id);

    // We have entered payload diff for the update. The data have been updated,
    // and we cannot find the type in catalog. This means we have some serious
    // data corruption bug(s).
    ASSERT_INVARIANT(
        type_record_id != c_invalid_gaia_id,
        gaia_fmt::format("The type '{}' does not exist in the catalog for payload diff!", type_id).c_str());

    auto schema = catalog_core::get_table(type_record_id).binary_schema();

    for (const auto& field_view : catalog_core::list_fields(type_record_id))
    {
        field_position_t field_position = field_view.position();

        if (!payload_types::are_field_values_equal(
                type_id, payload1, payload2, schema->data(), schema->size(), field_position))
        {
            changed_fields.push_back(field_position);
        }
    }
    return changed_fields;
}

} // namespace db
} // namespace gaia
