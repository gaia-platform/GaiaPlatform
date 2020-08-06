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

field_list_t compute_payload_diff(gaia_id_t type_id, const uint8_t* payload1, const uint8_t* payload2) {
    field_list_t retval(type_id);

    const vector<gaia_id_t>& fields = gaia::catalog::list_fields(type_id);
    size_t num_fields = fields.size();
    auto_transaction_t tx;

    for (field_position_t i = 0; i < num_fields; i++) {
        gaia::catalog::Gaia_table table = gaia::catalog::Gaia_table::get(type_id);
        string schema = gaia::catalog::get_bfbs(type_id);
        field_position_t pos = gaia::catalog::Gaia_field::get(fields[i]).position();

        data_holder_t data_holder1 = get_field_value(type_id, payload1, reinterpret_cast<const uint8_t*>(schema.c_str()), pos);
        data_holder_t data_holder2 = get_field_value(type_id, payload2, reinterpret_cast<const uint8_t*>(schema.c_str()), pos);

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
