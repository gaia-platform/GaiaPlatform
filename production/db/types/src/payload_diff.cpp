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
#include "gaia_catalog.hpp"
#include "catalog_gaia_generated.h"
#include "types.hpp"

namespace gaia
{
namespace db
{
namespace types
{

field_list_t compute_payload_diff(gaia_id_t table_id, const uint8_t* payload1, const uint8_t* payload2) {
    field_list_t retval(table_id);

    const vector<gaia_id_t>& fields = gaia::catalog::list_fields(table_id);
    size_t num_fields = fields.size();
    auto_transaction_t tx;

    for (field_position_t i = 0; i < num_fields; i++) {
        gaia::catalog::Gaia_table table = gaia::catalog::Gaia_table::get(table_id);
        gaia_id_t table_id = table.gaia_id();
        string schema = gaia::catalog::get_bfbs(table_id);
        data_holder_t data_holder1 = get_table_field_value(table_id, payload1, reinterpret_cast<const uint8_t*>(schema.c_str()), i);
        data_holder_t data_holder2 = get_table_field_value(table_id, payload2, reinterpret_cast<const uint8_t*>(schema.c_str()), i);

        // Compare values and set.
        if (data_holder1.compare(data_holder2) != 0) {
            retval.add(fields[i]);
        }
    }

    return retval;
}

}
}
}
