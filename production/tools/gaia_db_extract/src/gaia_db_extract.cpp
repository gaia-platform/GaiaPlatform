/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_db_extract.hpp"

#include <iostream>
#include <memory>
#include <vector>

#include "gaia/exceptions.hpp"

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
    json["id"] = field.gaia_id().value();
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
    json["id"] = table.gaia_id().value();
    json["type"] = table.type();

    for (const auto& field : table.gaia_fields())
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

    for (const auto& table : db.gaia_tables())
    {
        json["tables"].push_back(to_json(table));
    }

    return json;
}

// To make sure that gaia_db_extract() can issue a warning if not initialized.
static bool g_cache_initialized = false;

bool gaia_db_extract_initialize()
{
    begin_transaction();

    for (auto table_view : catalog_core::list_tables())
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

    g_cache_initialized = true;
    return g_cache_initialized;
}

static string dump_catalog()
{
    stringstream catalog_dump;
    json_t json;

    begin_transaction();

    for (const auto& db : gaia_database_t::list())
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

    commit_transaction();

    catalog_dump << json.dump(c_default_json_indentation);
    auto return_string = catalog_dump.str();
    if (!return_string.compare("null"))
    {
        return c_empty_object;
    }
    else
    {
        return return_string;
    }
}

// Select the correct data_holder_t element from which to pull the field value.
static bool add_field_value(json_t& row, const gaia_field_t& field_object, const data_holder_t& value)
{
    auto field_name = field_object.name();
    switch (value.type)
    {
    case reflection::String:
        // Null is possible.
        row[field_name] = (value.hold.string_value == nullptr) ? c_empty_string : value.hold.string_value;
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
        return false;
    }

    return true;
}

// Scan the database for rows within the database and table requested. Dump rows into JSON
// format, potentially restricted by start_after and row_limit parameters.
static string dump_rows(string database, string table, uint64_t start_after, uint32_t row_limit)
{
    bool terminate = false;
    bool has_included_top_level = false;
    table_iterator_t table_iterator;
    stringstream row_dump;
    json_t rows = json_t{};

    if (!g_cache_initialized)
    {
        fprintf(stderr, "API not initialized. Call gaia_db_extract_initialize() first.\n");
        return c_empty_object;
    }

    begin_transaction();

    // Locate the requested database.
    for (const auto& database_object : gaia_database_t::list().where(gaia_database_expr::name == database))
    {
        // Make sure the requested table is in this databaser..
        for (const auto& table_object : database_object.gaia_tables().where(gaia_table_expr::name == table))
        {
            gaia_type_t table_type = table_object.type();
            if (!table_iterator.initialize_scan(table_type, start_after))
            {
                terminate = true;
                break;
            }

            while (!table_iterator.has_scan_ended())
            {
                if (!has_included_top_level)
                {
                    // Top level of the JSON document, contains database and table names.
                    rows["database"] = database;
                    rows["table"] = table;
                    has_included_top_level = true;
                }

                // Note that a row_limit of -1 means "unlimited", so it will never be 0.
                if (row_limit-- == 0)
                {
                    terminate = true;
                    break;
                }

                json_t row;
                for (const auto& field_object : table_object.gaia_fields())
                {
                    // Pull one field value out of the database and included it in the JSON row object.
                    auto value = table_iterator.extract_field_value(field_object.repeated_count(), field_object.position());
                    add_field_value(row, field_object, value);
                }
                row["row_id"] = table_iterator.gaia_id().value();
                rows["rows"].push_back(row);

                // Next row.
                table_iterator.scan_forward();
            }
            if (terminate)
            {
                break;
            }
        }
        if (terminate)
        {
            break;
        }
    }

    commit_transaction();

    row_dump << rows.dump(c_default_json_indentation);
    auto return_string = rows.dump(c_default_json_indentation);
    if (!return_string.compare("null"))
    {
        return c_empty_object;
    }
    else
    {
        return return_string;
    }
}

string gaia_db_extract(string database, string table, uint64_t start_after, uint32_t row_limit)
{
    // Select the document. Either the catalog (no parameters) or table rows (database and table parameters).
    if (database.size() == 0 && table.size() == 0)
    {
        return dump_catalog();
    }
    else if (database.size() > 0 && table.size() > 0)
    {
        if (row_limit != 0)
        {
            return dump_rows(database, table, start_after, row_limit);
        }
        else
        {
            return c_empty_object;
        }
    }
    else
    {
        fprintf(stderr, "Must have both database name and table name to extract row data.\n");
        return c_empty_object;
    }
}

} // namespace db_extract
} // namespace tools
} // namespace gaia
