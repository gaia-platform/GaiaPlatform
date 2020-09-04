/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "payload_diff.hpp"

#include <vector>
#include <string>

#include "auto_transaction.hpp"
#include "data_holder.hpp"
#include "field_access.hpp"
#include "gaia_catalog.h"
#include "gaia_catalog.hpp"
#include "system_table_types.hpp"

namespace gaia {
namespace db {
namespace types {

// TODO: add unit tests for this function
void compute_payload_diff(gaia_id_t type_id, const uint8_t *payload1, const uint8_t *payload2, field_position_list_t *changed_fields) {
    // Make sure caller passes valid pointer to changed_fields.
    assert(changed_fields);
    // Query the catalog for the schema
    for (auto table = gaia::catalog::gaia_table_t::get_first();
         table;
         table = table.get_next()) {
        if (table.type() == type_id) {

            string schema = gaia::catalog::get_bfbs(table.gaia_id());
            for (auto field : table.gaia_field_list()) {
                if (field.type() != static_cast<uint8_t>(gaia::catalog::data_type_t::e_references)) {
                    field_position_t pos = field.position();
                    data_holder_t data_holder1 = get_field_value(
                        type_id, payload1, reinterpret_cast<const uint8_t *>(schema.c_str()), pos);
                    data_holder_t data_holder2 = get_field_value(
                        type_id, payload2, reinterpret_cast<const uint8_t *>(schema.c_str()), pos);

                    // Compare values and set.
                    if (data_holder1.compare(data_holder2) != 0) {
                        changed_fields->push_back(pos);
                    }
                }
            }
        }
    }
}

} // namespace types
} // namespace db
} // namespace gaia
