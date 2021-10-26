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
#include "table_iterator.hpp"

using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace gaia::tools::db_extract;
using namespace gaia::db::payload_types;
using namespace std;
using json_t = nlohmann::json;

namespace gaia
{
namespace tools
{
namespace db_extract
{

constexpr int c_default_json_indentation = 4;

// Add a field array element.
static json_t to_json(gaia_field_t field)
{
    json_t json;

    json["name"] = field.name();
    json["id"] = field.gaia_id();
    json["position"] = field.position();
    json["repeated_count"] = field.repeated_count();
    json["type"] = get_data_type_name(data_type_t(field.type()));

    return json;
}

// Add a table array element.
static json_t to_json(gaia_table_t table)
{
    json_t json;

    json["name"] = table.name();
    json["id"] = table.gaia_id();
    json["type"] = table.type();

    for (auto field : table.gaia_fields())
    {
        json["fields"].push_back(to_json(field));
    }

    return json;
}

// Add a database array element.
static json_t to_json(gaia_database_t db)
{
    json_t json;

    json["name"] = db.name();

    for (auto table : db.gaia_tables())
    {
        json["tables"].push_back(to_json(table));
    }

    return json;
}

// To make sure that gaia_db_extract() can issue a warning if not initialized.
static bool s_cache_initialized = false;

bool gaia_db_extract_initialize()
{
    begin_transaction();

    for (auto table_view : catalog_core_t::list_tables())
    {
        string table_name(table_view.name());

        auto type_information = make_unique<type_information_t>();

        initialize_type_information_from_binary_schema(
            type_information.get(),
            table_view.binary_schema()->data(),
            table_view.binary_schema()->size());

        type_information.get()->set_serialization_template(
            table_view.serialization_template()->data(),
            table_view.serialization_template()->size());

        // It's okay for the information to exist after the first initialization.
        type_cache_t::get()->set_type_information(table_view.table_type(), type_information);
    }

    commit_transaction();

    s_cache_initialized = true;
    return s_cache_initialized;
}

string gaia_db_extract(string database, string table, uint64_t start_after, uint32_t row_limit)
{
    stringstream catalog_dump;
    json_t json;

    begin_transaction();

    for (auto db : gaia_database_t::list())
    {
        // The nameless database is "default" for tables that are not in a named database, and
        // "catalog" is the name of the catalog database. If you don't want one or both of these
        // to be part of the catalog extraction, uncomment them.
        if (/*strlen(db.name()) == 0 || !strcmp(db.name(), "catalog") ||*/ !strcmp(db.name(), "()"))
        {
            continue;
        }

        json["databases"].push_back(to_json(db));
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
    table_iterator_t table_iterator;
    stringstream row_dump;
    json_t rows = json_t{};

    if (!s_cache_initialized)
    {
        fprintf(stderr, "API not initialized. Call gaia_db_extract_initialize first.\n");
        return "{}";
    }

    for (auto& json_databases : json["databases"])
    {
        if (!json_databases["name"].get<string>().compare(database))
        {
            for (auto& json_tables : json_databases["tables"])
            {
                if (!json_tables["name"].get<string>().compare(table))
                {
                    gaia_type_t table_type = json_tables["type"].get<uint32_t>();
                    if (!table_iterator.initialize_scan(table_type, start_after))
                    {
                        terminate = true;
                        break;
                    }
                    rows["database"] = database;
                    rows["table"] = table;
                    while (!table_iterator.has_scan_ended())
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
                            auto value = table_iterator.extract_field_value(field["repeated_count"].get<uint16_t>(), field["position"].get<uint32_t>());
                            auto field_type = field["type"].get<string>();
                            auto field_name = field["name"].get<string>();
                            row["row_id"] = table_iterator.gaia_id();
                            switch (value.type)
                            {
                            case reflection::String:
                                row[field_name] = value.hold.string_value;
                                break;

                            case reflection::Float:
                            case reflection::Double:
                                row[field_name] = value.hold.float_value;
                                break;

                            // The remainder are integers.
                            case reflection::Bool:
                            case reflection::Byte:
                            case reflection::UByte:
                            case reflection::Short:
                            case reflection::UShort:
                            case reflection::Int:
                            case reflection::UInt:
                            case reflection::Long:
                            case reflection::ULong:
                                row[field_name] = value.hold.integer_value;
                                break;

                            default:
                                fprintf(stderr, "Unhandled data_holder_t type '%d'.\n", value.type);
                                break;
                            }
                        }
                        rows["rows"].push_back(row);
                        table_iterator.scan_forward();
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
