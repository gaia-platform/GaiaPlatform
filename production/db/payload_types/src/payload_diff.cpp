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
#include "system_table_types.hpp"

namespace gaia
{
namespace db
{
namespace payload_types
{

using namespace gaia::catalog;

// TODO: add unit tests for this function
void compute_payload_diff(gaia_type_t type_id, const uint8_t* payload1, const uint8_t* payload2,
                          field_position_list_t* changed_fields)
{
    // Make sure caller passes valid pointer to changed_fields.
    assert(changed_fields);
    auto table = gaia_table_t::get(ddl_executor_t::get().find_table_id(type_id));
    string schema = get_bfbs(table.gaia_id());
    for (const auto& field : table.gaia_field_list())
    {
        if (field.type() != static_cast<uint8_t>(data_type_t::e_references))
        {
            field_position_t pos = field.position();
            data_holder_t data_holder1
                = get_field_value(type_id, payload1, reinterpret_cast<const uint8_t*>(schema.c_str()), pos);
            data_holder_t data_holder2
                = get_field_value(type_id, payload2, reinterpret_cast<const uint8_t*>(schema.c_str()), pos);

            // Compare values and set.
            if (data_holder1.compare(data_holder2) != 0)
            {
                changed_fields->push_back(pos);
            }
        }
    }
}

} // namespace payload_types
} // namespace db
} // namespace gaia
