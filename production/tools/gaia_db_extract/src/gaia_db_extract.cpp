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
#include "gaia_internal/db/gaia_ptr.hpp"

#include "json.hpp"
#include "scan_state.hpp"

using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace gaia::tools::db_extract;
using namespace std;
using json_t = nlohmann::json;

namespace gaia
{
namespace tools
{
namespace db_extract
{

json_t marshall_field(gaia_field_t field)
{
    json_t json_object;

    json_object["name"] = field.name();
    json_object["id"] = field.gaia_id();
    json_object["position"] = field.position();
    json_object["repeated_count"] = field.repeated_count();
    json_object["type"] = get_data_type_name(data_type_t(field.type()));

    return json_object;
}

json_t marshall_table(gaia_table_t table)
{
    json_t json_object;

    json_object["name"] = table.name();
    json_object["id"] = table.gaia_id();
    json_object["type"] = table.type();

    for (auto field : table.gaia_fields())
    {
        json_object["fields"].push_back(marshall_field(field));
    }

    return json_object;
}

json_t marshall_database(gaia_database_t db)
{
    json_t json_object;

    json_object["name"] = db.name();

    for (auto table : db.gaia_tables())
    {
        json_object["tables"].push_back(marshall_table(table));
    }

    return json_object;
}

string gaia_db_extract(string database, string table, uint64_t start_after, uint32_t row_limit)
{
    stringstream catalog_dump;
    json_t json_object;

    begin_transaction();

    for (auto db : gaia_database_t::list())
    {
        if (/*strlen(db.name()) == 0 || !strcmp(db.name(), "catalog") ||*/ !strcmp(db.name(), "()"))
        {
            continue;
        }

        json_object["databases"].push_back(marshall_database(db));
    }

    // If a database and table have been specified, move ahead to extract the row data.
    if (database.size() == 0 || table.size() == 0)
    {
        commit_transaction();
        catalog_dump << json_object.dump(4);
        auto return_string = catalog_dump.str();
        if (!return_string.compare("null"))
        {
            return "{}";
        }
        else
        {
            return return_string;
        }
    }

    bool terminate = false;
    scan_state_t scan_state;
    stringstream row_dump;
    json_t rows = json_t{};

    for (auto& json_databases : json_object["databases"])
    {
        if (!json_databases["name"].get<string>().compare(database))
        {
            for (auto& json_tables : json_databases["tables"])
            {
                if (!json_tables["name"].get<string>().compare(table))
                {
                    gaia_type_t table_type = json_tables["type"].get<uint32_t>();
                    if (!scan_state.initialize_scan(table_type, start_after))
                    {
                        terminate = true;
                        break;
                    }
                    rows["database"] = database;
                    rows["table"] = table;
                    while (!scan_state.has_scan_ended())
                    {
                        // Note that a row_limit of -1 means "unlimited", so it will never be 0.
                        if (row_limit-- == 0)
                        {
                            terminate = true;
                            break;
                        }
                        json_t row;
                        for (auto& f : json_tables["fields"])
                        {
                            auto value = scan_state.extract_field_value(f["repeated_count"].get<uint16_t>(), f["position"].get<uint32_t>());
                            if (!value.isnull)
                            {
                                auto field_type = f["type"].get<string>();
                                auto field_name = f["name"].get<string>();
                                row["row_id"] = scan_state.gaia_id();
                                if (!field_type.compare("string"))
                                {
                                    row[field_name] = (char*)value.value;
                                }
                                else
                                {
                                    row[field_name] = value.value;
                                }
                            }
                        }
                        rows["rows"].push_back(row);
                        scan_state.scan_forward();
                    }
                    if (terminate)
                    {
                        break;
                    }
                }
            }
            if (terminate)
            {
                break;
            }
        }
    }

    commit_transaction();

    row_dump << rows.dump(4);
    auto return_string = rows.dump(4);
    if (!return_string.compare("null"))
    {
        return "{}";
    }
    else
    {
        return return_string;
    }
}

} // namespace db_extract
} // namespace tools
} // namespace gaia
