/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_db_extract.hpp"

#include <memory>
#include <vector>

#include "gaia/db/db.hpp"

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "json.hpp"

using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace std;
using json = nlohmann::json;

json print_field(gaia::catalog::gaia_field_t field)
{
    json j;

    j["name"] = field.name();
    j["type"] = field.type();
    j["id"] = field.gaia_id();

    return j;
}

json print_table(gaia::catalog::gaia_table_t table)
{
    json j;

    j["name"] = table.name();
    j["id"] = table.gaia_id();

    for (auto field : table.gaia_fields())
    {
        j["fields"].push_back(print_field(field));
    }

    return j;
}

json print_database(gaia::catalog::gaia_database_t db)
{
    json j;

    j["name"] = db.name();

    for (auto table : db.gaia_tables())
    {
        j["tables"].push_back(print_table(table));
    }

    return j;
}

string gaia_db_extract(string database, string table, gaia_id_t start_after, uint32_t row_limit)
{
    stringstream dump;
    json j;

    begin_transaction();

    if (database.size() == 0)
    {
        for (auto db : gaia::catalog::gaia_database_t::list())
        {
            if (strlen(db.name()) == 0 || !strcmp(db.name(), "catalog") || !strcmp(db.name(), "()"))
            {
                continue;
            }
            j["databases"].push_back(print_database(db));
        }
    }
    else
    {
        // Dump table from database.
    }

    commit_transaction();

    // Serialize the json object.
    dump << j.dump(4);

    return dump.str();
}
