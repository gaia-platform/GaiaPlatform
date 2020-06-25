/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "catalog_gaia_generated.h"

namespace gaia {
namespace catalog {

typedef unordered_map<gaia_id_t, std::string> table_id_map_t;
typedef std::pair<int, std::string> fieldstr_pair_t;
typedef std::unordered_map<std::string, vector<fieldstr_pair_t>> table_collection_t;

static std::string get_data_type_name(gaia_data_type e) {
    string name{EnumNamegaia_data_type(e)};
    // Convert the data type enum name string to lowercase case
    // because FlatBuffers schema is case sensitive
    // and does not recognize uppercase keywords.
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return name;
}

static std::string generate_field(unique_ptr<Gaia_field> &field) {

    string field_name{field->name()};

    string field_type;
    field_type = get_data_type_name(field->type());

    if (field->repeated_count() == 1) {
        return field_name + ":" + field_type;
    } else if (field->repeated_count() == 0) {
        return field_name + ":[" + field_type + "]";
    } else {
        return field_name +
               ":[" + field_type + ":" + to_string(field->repeated_count()) + "]";
    }
}

std::string generate_fbs() {
    table_id_map_t table_id_map;
    table_collection_t tables;

    gaia::db::begin_transaction();
    for (unique_ptr<Gaia_table> t{Gaia_table::get_first()}; t; t.reset(t->get_next())) {
        table_id_map[t->gaia_id()] = t->name();
    }

    for (unique_ptr<Gaia_field> field{Gaia_field::get_first()}; field; field.reset(field->get_next())) {
        fieldstr_pair_t fieldstr_pair = std::make_pair(
            field->position(),
            generate_field(field));

        const std::string &table_name = table_id_map[field->table_id()];

        if (tables.find(table_name) == tables.end()) {
            tables[table_name] = {fieldstr_pair};
        } else {
            auto &v = tables[table_name];
            auto pos = std::lower_bound(v.begin(), v.end(), fieldstr_pair,
                [](fieldstr_pair_t lhs, fieldstr_pair_t rhs) -> bool { return lhs.first < rhs.first; });
            tables[table_name].insert(pos, fieldstr_pair);
        }
    }
    gaia::db::commit_transaction();

    std::string fbs;
    for (auto kv : tables) {
        fbs += "table " + kv.first + "{\n";
        for (auto field_pair : kv.second) {
            fbs += "\t" + field_pair.second + ";\n";
        }
        fbs += "}\n\n";
    }
    return fbs;
}
} // namespace catalog
} // namespace gaia
