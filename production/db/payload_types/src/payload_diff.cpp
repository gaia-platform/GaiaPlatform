/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "payload_diff.hpp"

#include <string>
#include <vector>

#include "auto_transaction.hpp"
#include "data_holder.hpp"
#include "ddl_executor.hpp"
#include "field_access.hpp"
#include "gaia_catalog.h"
#include "gaia_catalog.hpp"
#include "gaia_common.hpp"
#include "gaia_se_catalog.hpp"
#include "retail_assert.hpp"
#include "system_table_types.hpp"

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

    auto table = gaia::catalog::gaia_table_t::get(gaia::catalog::ddl_executor_t::get().find_table_id(type_id));

    auto schema = gaia_catalog_t::get_bfbs(table.gaia_id());

    for (auto field : gaia_catalog_t::list_fields(table.gaia_id()))
    {
        if (field.type() == data_type_t::e_references)
        {
            continue;
        }
        field_position_t pos = field.position();
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
