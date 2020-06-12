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

typedef unordered_map<gaia_id_t, std::string> type_id_map_t;
typedef std::pair<int, std::string> fieldstr_pair_t;
typedef std::unordered_map<std::string, vector<fieldstr_pair_t>> type_collection_t;

static std::string generate_field(unique_ptr<GaiaField> &field, type_id_map_t &type_id_map) {
    string field_name{field->name()};

    if (field->repeated_count() == 1) {
        return field_name + ":" + type_id_map[field->type_id()];
    } else if (field->repeated_count() == 0) {
        return field_name + ":[" + type_id_map[field->type_id()] + "]";
    } else {
        return field_name +
               ":[" + type_id_map[field->type_id()] +
               ":" + to_string(field->repeated_count()) + "]";
    }
}

std::string generate_fbs() {
    type_id_map_t type_id_map;
    type_collection_t types;

    gaia::db::begin_transaction();
    for (unique_ptr<GaiaType> t{GaiaType::get_first()}; t; t.reset(t->get_next())) {
        string type_name{t->name()};
        std::transform(type_name.begin(), type_name.end(), type_name.begin(), ::tolower);
        type_id_map[t->gaia_id()] = type_name;
    }

    for (unique_ptr<GaiaField> f{GaiaField::get_first()}; f; f.reset(f->get_next())) {
        auto e = std::make_pair(f->position(), generate_field(f, type_id_map));
        if (types.find(type_id_map[f->owner_id()]) == types.end()) {
            types[type_id_map[f->owner_id()]] = {e};
        } else {
            auto &v = types[type_id_map[f->owner_id()]];
            auto pos = std::lower_bound(v.begin(), v.end(), e,
                [](fieldstr_pair_t lhs, fieldstr_pair_t rhs) -> bool { return lhs.first < rhs.first; });
            types[type_id_map[f->owner_id()]].insert(pos, e);
        }
    }
    gaia::db::commit_transaction();

    std::string fbs;
    for (auto kv : types) {
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
