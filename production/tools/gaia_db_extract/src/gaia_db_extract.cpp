/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_db_extract.hpp"

#include <iostream>
#include <memory>
#include <vector>

#include "gaia/db/db.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "scan_state.hpp"

using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace gaia::extract;
using namespace std;
using json = nlohmann::json;

json print_field(gaia_field_t field)
{
    json j;

    j["name"] = field.name();
    j["id"] = field.gaia_id();
    j["position"] = field.position();
    j["repeated_count"] = field.repeated_count();
    j["type"] = get_data_type_name(data_type_t(field.type()));

    return j;
}

json print_table(gaia_table_t table)
{
    json j;

    j["name"] = table.name();
    j["id"] = table.gaia_id();
    j["type"] = table.type();

    for (auto field : table.gaia_fields())
    {
        j["fields"].push_back(print_field(field));
    }

    return j;
}

json print_database(gaia_database_t db)
{
    json j;

    j["name"] = db.name();

    for (auto table : db.gaia_tables())
    {
        j["tables"].push_back(print_table(table));
    }

    return j;
}

json gaia_db_extract(string database, string table, gaia_id_t start_after, uint32_t row_limit)
{
    stringstream dump;
    json j;

    begin_transaction();

    for (auto db : gaia_database_t::list())
    {
        if (strlen(db.name()) == 0 || /*!strcmp(db.name(), "catalog") ||*/ !strcmp(db.name(), "()"))
        {
            continue;
        }

        j["databases"].push_back(print_database(db));
    }
    dump << j.dump(4);
    fprintf(stderr, "%s\n", dump.str().c_str());

    scan_state_t scan_state;

    fprintf(stderr, "database name: %s, table name: %s\n", database.c_str(), table.c_str());
    for (auto& J : j["databases"])
    {
        if (J["name"].get<string>().compare(database) == 0)
        {
            for (auto& JJ : J["tables"])
            {
                if (JJ["name"].get<string>().compare(table) == 0)
                {
                    gaia_type_t table_type = JJ["type"].get<uint32_t>();
                    scan_state.initialize_scan(table_type, JJ["fields"]);
                    while (!scan_state.has_scan_ended())
                    {
                        for (auto& f : JJ["fields"])
                        {
                            auto value = scan_state.extract_field_value(f["repeated_count"].get<uint16_t>(), f["position"].get<uint32_t>());
                            if (!value.isnull)
                            {
                                auto field_type = f["type"].get<string>();
                                fprintf(stderr, "%s = %s:", f["name"].get<string>().c_str(), field_type.c_str());
                                if (field_type.compare("string") == 0)
                                {
                                    fprintf(stderr, "%s | ", (char*)value.value);
                                }
                                else
                                {
                                    fprintf(stderr, "%lu | ", value.value);
                                }
                            }
                        }
                        fprintf(stderr, "\n");
                        scan_state.scan_forward();
                    }
                }
            }
        }
    }

    commit_transaction();

    // Serialize the json object.
    // dump << j.dump(4);

    return dump.str();
}
