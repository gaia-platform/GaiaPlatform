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
#include "gaia_catalog.h"
#include "types.hpp"

namespace gaia
{
namespace db
{
namespace types
{

field_list_t compute_payload_diff(gaia_id_t type_id, const uint8_t* payload1, const uint8_t* payload2, bool outside_tx) {
    field_list_t retval(type_id);

    const vector<gaia_id_t>& fields = gaia::catalog::list_fields(type_id);
    size_t num_fields = fields.size();

    if (outside_tx) {
        auto_transaction_t tx;
    }

    for (field_position_t i = 0; i < num_fields; i++) {
        string schema = gaia::catalog::get_bfbs(type_id, outside_tx);
        auto c = gaia::catalog::gaia_field_t::get(fields[i]);
        field_position_t pos = c.position();

        data_holder_t data_holder1 = get_table_field_value(type_id, payload1, reinterpret_cast<const uint8_t*>(schema.c_str()), pos);
        data_holder_t data_holder2 = get_table_field_value(type_id, payload2, reinterpret_cast<const uint8_t*>(schema.c_str()), pos);

        // Compare values and set.
        if (data_holder1.compare(data_holder2) != 0) {
            retval.add(fields[i]);
        }
    }

    return retval;
}

shared_ptr<vector<field_position_t>> compute_payload_position_diff(gaia_id_t type_id, const uint8_t* payload1, const uint8_t* payload2) {
    field_list_t diff = compute_payload_diff(type_id, payload1, payload2, false);
    string schema = gaia::catalog::get_bfbs(type_id, false);

    auto retval = std::make_shared<vector<field_position_t>>();

    for (size_t i = 0; i < diff.size(); i++) {
        retval.get()->push_back(gaia::catalog::gaia_field_t::get(diff[i]).position());
    }

    return retval;
}

}
}
}
