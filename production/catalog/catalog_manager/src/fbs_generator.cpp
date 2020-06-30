/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "catalog_manager.hpp"
#include "catalog_gaia_generated.h"

namespace gaia {
namespace catalog {

static string get_data_type_name(gaia_data_type e) {
    string name{EnumNamegaia_data_type(e)};
    // Convert the data type enum name string to lowercase case
    // because FlatBuffers schema is case sensitive
    // and does not recognize uppercase keywords.
    transform(name.begin(), name.end(), name.begin(), ::tolower);
    return name;
}

static string generate_field(const Gaia_field &field) {
    string field_name{field.name()};

    string field_type;
    field_type = get_data_type_name(field.type());

    if (field.repeated_count() == 1) {
        return field_name + ":" + field_type;
    } else if (field.repeated_count() == 0) {
        return field_name + ":[" + field_type + "]";
    } else {
        return field_name +
               ":[" + field_type + ":" + to_string(field.repeated_count()) + "]";
    }
}

string generate_fbs() {
    string fbs;
    gaia::db::begin_transaction();
    for (gaia_id_t table_id : catalog_manager_t::get().list_tables()) {
        unique_ptr<Gaia_table> table{Gaia_table::get_row_by_id(table_id)};
        fbs += "table " + string(table->name()) + "{\n";
        for (gaia_id_t field_id : catalog_manager_t::get().list_fields(table_id)) {
            unique_ptr<Gaia_field> field{Gaia_field::get_row_by_id(field_id)};
            fbs += "\t" + generate_field(*field) + ";\n";
        }
        fbs += "}\n\n";
    }
    gaia::db::commit_transaction();
    return fbs;
}
} // namespace catalog
} // namespace gaia
