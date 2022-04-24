/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_db_extract.hpp"

#include <iostream>
#include <memory>
#include <vector>

#include <json.hpp>

#include "gaia/exceptions.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

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
    json["hash"] = field.hash();

    return json;
}

// Add a link array element.
static json_t to_json(gaia_relationship_t relationship, bool is_outgoing)
{
    json_t json;

    json["is_outgoing"] = is_outgoing;
    if (is_outgoing)
    {
        json["link_name"] = relationship.to_child_link_name();
        json["table_name"] = relationship.child().name();
    }
    else
    {
        json["link_name"] = relationship.to_parent_link_name();
        json["table_name"] = relationship.parent().name();
    }

    return json;
}

// Add a table array element.
static json_t to_json(gaia_table_t table)
{
    json_t json;

    json["name"] = table.name();
    json["id"] = table.gaia_id().value();
    json["hash"] = table.hash();
    json["type"] = table.type();

    for (const auto& field : table.gaia_fields())
    {
        json["fields"].push_back(to_json(field));
    }

    for (const auto& relationship : gaia_relationship_t::list())
    {
        if (strcmp(relationship.parent().name(), table.name()) == 0)
        {
            json["relationships"].push_back(to_json(relationship, true));
        }
        else if (strcmp(relationship.child().name(), table.name()) == 0)
        {
            json["relationships"].push_back(to_json(relationship, false));
        }
    }

    return json;
}

// Add a database array element.
static json_t to_json(gaia_database_t db)
{
    json_t json;

    json["name"] = db.name();
    json["hash"] = db.hash();

    for (const auto& table : db.gaia_tables())
    {
        json["tables"].push_back(to_json(table));
    }

    return json;
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

static bool add_field_array_value(json_t& row, const gaia_field_t& field_object, const std::vector<data_holder_t>& value)
{
    auto field_name = field_object.name();
    if (value.empty())
    {
        row[field_name] = std::vector<int>();
        return true;
    }

    switch (value[0].type)
    {
    case reflection::Float:
    case reflection::Double:
    {
        std::vector<double> values;
        values.reserve(value.size());
        for (const auto& element : value)
        {
            values.push_back(element.hold.float_value);
        }
        row[field_name] = values;
        break;
    }

    // The remainder are integers.
    case reflection::Bool:
    {
        std::vector<bool> values;
        values.reserve(value.size());
        for (const auto& element : value)
        {
            values.push_back(element.hold.integer_value != 0);
        }
        row[field_name] = values;
        break;
    }
    case reflection::Byte:
    case reflection::UByte:
    case reflection::Short:
    case reflection::UShort:
    case reflection::Int:
    case reflection::UInt:
    case reflection::Long:
    case reflection::ULong:
    {
        std::vector<int64_t> values;
        values.reserve(value.size());
        for (const auto& element : value)
        {
            values.push_back(element.hold.integer_value);
        }
        row[field_name] = values;
        break;
    }
    default:
        fprintf(stderr, "Unhandled data_holder_t type '%d' for field '%s'.\n", value[0].type, field_name);
        return false;
    }

    return true;
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
        fprintf(stderr, "Unhandled data_holder_t type '%d' for field '%s'.\n", value.type, field_name);
        return false;
    }

    return true;
}

static json_t serialize_row(table_iterator_t& table_iterator, const gaia_table_t& table_object)
{
    json_t row;
    for (const auto& field_object : table_object.gaia_fields())
    {
        // Pull one field value out of the database and included it in the JSON row object.
        if (field_object.repeated_count() == 1)
        {
            auto value = table_iterator.extract_field_value(field_object.position());
            add_field_value(row, field_object, value);
        }
        else
        {
            auto value = table_iterator.extract_field_vector_value(field_object.position());
            add_field_array_value(row, field_object, value);
        }
    }
    row["row_id"] = table_iterator.gaia_id().value();
    return row;
}

// Scan the database for rows within the database and table requested. Dump rows into JSON
// format, potentially restricted by start_after and row_limit parameters.
static string dump_rows(string database, string table, uint64_t start_after, uint32_t row_limit, string link_name, uint64_t anchor_record)
{
    bool terminate = false;
    bool has_included_top_level = false;
    table_iterator_t table_iterator;
    stringstream row_dump;
    json_t rows = json_t{};

    begin_transaction();

    // Locate the requested database.
    for (const auto& database_object : gaia_database_t::list().where(gaia_database_expr::name == database))
    {
        // Make sure the requested table is in this database.
        for (const auto& table_object : database_object.gaia_tables().where(gaia_table_expr::name == table))
        {
            if (!link_name.empty())
            {
                gaia_relationship_t relationship;
                string table_name;
                bool is_from_parent = false;
                for (const gaia_relationship_t& child_relationship : table_object.incoming_relationships())
                {
                    if (strcmp(link_name.c_str(), child_relationship.to_parent_link_name()) == 0)
                    {
                        relationship = child_relationship;
                        table_name = relationship.parent().name();
                        is_from_parent = false;
                        break;
                    }
                }

                if (!relationship)
                {
                    for (const gaia_relationship_t& child_relationship : table_object.outgoing_relationships())
                    {
                        if (strcmp(link_name.c_str(), child_relationship.to_child_link_name()) == 0)
                        {
                            is_from_parent = true;
                            relationship = child_relationship;
                            table_name = relationship.child().name();
                            break;
                        }
                    }
                }
                if (!relationship)
                {
                    terminate = true;
                    fprintf(stderr, "The relationship %s doesn't exist\n", link_name.c_str());
                    break;
                }

                std::vector<gaia_id_t> linked_record_ids;

                auto anchor_row = gaia_ptr_t::from_gaia_id(anchor_record);
                if (!anchor_row)
                {
                    terminate = true;
                    break;
                }
                if (is_from_parent)
                {
                    size_t link_counter = 0;

                    auto reference_id = anchor_row.references()[relationship.first_child_offset()];
                    if (reference_id == c_invalid_gaia_id)
                    {
                        break;
                    }
                    auto reference = gaia_ptr_t::from_gaia_id(reference_id);
                    auto child_id = reference.references()[1];
                    while (true)
                    {
                        if (child_id == c_invalid_gaia_id)
                        {
                            break;
                        }

                        linked_record_ids.push_back(child_id);

                        if (row_limit != c_row_limit_unlimited && row_limit <= ++link_counter)
                        {
                            break;
                        }

                        auto child = gaia_ptr_t::from_gaia_id(child_id);
                        if (!child)
                        {
                            break;
                        }

                        child_id = child.references()[relationship.next_child_offset()];
                    }
                }
                else
                {
                    auto reference_id = anchor_row.references()[relationship.parent_offset()];
                    if (reference_id == c_invalid_gaia_id)
                    {
                        break;
                    }
                    auto reference = gaia_ptr_t::from_gaia_id(reference_id);
                    linked_record_ids.push_back(reference.references()[0]);
                }

                if (start_after != 0)
                {
                    auto iterator = linked_record_ids.begin();
                    while (iterator != linked_record_ids.end())
                    {
                        if (*iterator == start_after)
                        {
                            linked_record_ids.erase(iterator);
                            break;
                        }
                        iterator = linked_record_ids.erase(iterator);
                    }
                }

                for (const auto& linked_table_object : database_object.gaia_tables().where(gaia_table_expr::name == table_name))
                {
                    gaia_id_t table_id = linked_table_object.gaia_id();
                    gaia_type_t table_type = linked_table_object.type();
                    for (auto record_id : linked_record_ids)
                    {
                        if (!table_iterator.set(table_id, table_type, record_id))
                        {
                            break;
                        }
                        if (!has_included_top_level)
                        {
                            // Top level of the JSON document, contains database and table names.
                            rows["database"] = database;
                            rows["table"] = table_name;
                            has_included_top_level = true;
                        }
                        rows["rows"].push_back(serialize_row(table_iterator, linked_table_object));
                    }
                }

                terminate = true;
                break;
            }
            else
            {
                gaia_id_t table_id = table_object.gaia_id();
                gaia_type_t table_type = table_object.type();
                if (!table_iterator.initialize_scan(table_id, table_type, start_after))
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

                    rows["rows"].push_back(serialize_row(table_iterator, table_object));

                    // Next row.
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

string gaia_db_extract(string database, string table, uint64_t start_after, uint32_t row_limit, string link_name, uint64_t anchor_record)
{
    // Select the document. Either the catalog (no parameters) or table rows (database and table parameters).
    if (database.empty() && table.empty())
    {
        return dump_catalog();
    }
    else if (!table.empty())
    {
        // If no database name is specified then it will use the default name of ''.
        if ((link_name.empty() && anchor_record > 0) || (!link_name.empty() && anchor_record == 0))
        {
            fprintf(stderr, "Must have both link name name and anchor row id to extract related row data.\n");
            return c_empty_object;
        }
        if (row_limit != 0)
        {
            return dump_rows(database, table, start_after, row_limit, link_name, anchor_record);
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
