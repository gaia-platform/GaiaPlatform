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

constexpr int c_default_json_indentation = 4;

json_t to_field(gaia_field_t field)
{
    json_t json;

    json["name"] = field.name();
    json["id"] = field.gaia_id();
    json["position"] = field.position();
    json["repeated_count"] = field.repeated_count();
    json["type"] = get_data_type_name(data_type_t(field.type()));

    return json;
}

json_t to_table(gaia_table_t table)
{
    json_t json;

    json["name"] = table.name();
    json["id"] = table.gaia_id();
    json["type"] = table.type();

    for (auto field : table.gaia_fields())
    {
        json["fields"].push_back(to_field(field));
    }

    return json;
}

json_t to_database(gaia_database_t db)
{
    json_t json;

    json["name"] = db.name();

    for (auto table : db.gaia_tables())
    {
        json["tables"].push_back(to_table(table));
    }

    return json;
}

string gaia_db_extract(string database, string table, uint64_t start_after, uint32_t row_limit)
{
    stringstream catalog_dump;
    json_t json;

    begin_transaction();

    for (auto db : gaia_database_t::list())
    {
        if (/*strlen(db.name()) == 0 || !strcmp(db.name(), "catalog") ||*/ !strcmp(db.name(), "()"))
        {
            continue;
        }

        json["databases"].push_back(to_database(db));
    }

    // If a database and table have been specified, move ahead to extract the row data.
    if (database.size() == 0 || table.size() == 0)
    {
        commit_transaction();
        catalog_dump << json.dump(c_default_json_indentation);
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

    for (auto& json_databases : json["databases"])
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
                        for (auto& field : json_tables["fields"])
                        {
                            auto value = scan_state.extract_field_value(field["repeated_count"].get<uint16_t>(), field["position"].get<uint32_t>());
                            if (!value.is_null)
                            {
                                auto field_type = field["type"].get<string>();
                                auto field_name = field["name"].get<string>();
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

    row_dump << rows.dump(c_default_json_indentation);
    auto return_string = rows.dump(c_default_json_indentation);
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
