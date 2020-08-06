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
#include "types.hpp"

namespace gaia
{
namespace db
{
namespace types
{

field_list_t compute_payload_diff(gaia_id_t type_id, const uint8_t* payload1, const uint8_t* payload2) {
    field_list_t retval(type_id);

    auto_transaction_t tx;

    // Query the catalog for the schema
    gaia::catalog::gaia_table_t table = gaia::catalog::gaia_table_t::get(type_id);
    string schema = gaia::catalog::get_bfbs(type_id);

    for (auto field = gaia::catalog::gaia_field_t::get_first(); field; field.get_next()) {
        if (field.table_id() == type_id) {
            field_position_t pos = field.position();

            data_holder_t data_holder1 = get_table_field_value(type_id, payload1, reinterpret_cast<const uint8_t*>(schema.c_str()), pos);
            data_holder_t data_holder2 = get_table_field_value(type_id, payload2, reinterpret_cast<const uint8_t*>(schema.c_str()), pos);

            // Compare values and set.
            if (data_holder1.compare(data_holder2) != 0) {
                retval.add(pos);
            }
        }
    }

    return retval;
}

}
}
}
