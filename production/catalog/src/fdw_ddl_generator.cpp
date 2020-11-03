/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "fdw_ddl_generator.hpp"

#include <algorithm>
#include <sstream>
#include <string>

#include "gaia_catalog.h"
#include "retail_assert.hpp"

namespace gaia
{
namespace catalog
{

/**
 * Helper functions
 **/

static string generate_fdw_ddl_field(const string& name, const string& type, int count)
{
    if (count == 1)
    {
        return name + " " + type;
    }
    else
    {
        // Handle arrays as bytea for now.
        return name + " bytea";
    }
}

string get_fdw_data_type_name(data_type_t data_type)
{
    switch (data_type)
    {
    case data_type_t::e_bool:
        return "boolean";

    case data_type_t::e_int8:
    case data_type_t::e_uint8:
    case data_type_t::e_int16:
    case data_type_t::e_uint16:
        return "smallint";

    case data_type_t::e_int32:
    case data_type_t::e_uint32:
        return "integer";

    case data_type_t::e_int64:
    case data_type_t::e_uint64:
    case data_type_t::e_references:
        return "bigint";

    case data_type_t::e_float:
        return "real";

    case data_type_t::e_double:
        return "double precision";

    case data_type_t::e_string:
        return "text";

    default:
        stringstream message;
        message
            << "Unhandled data_type_t value " << static_cast<int>(data_type)
            << " in get_fdw_data_type_name()!";
        // If we use retail_assert(false), the compiler can't figure out
        // that it will throw an exception and will warn us about
        // potentially exiting the method without returning a value.
        throw retail_assertion_failure(message.str());
    }
}

static string generate_fdw_ddl_field(const gaia_field_t& field)
{
    string name{field.name()};
    string type{get_fdw_data_type_name(static_cast<data_type_t>(field.type()))};
    return generate_fdw_ddl_field(name, type, field.repeated_count());
}

string generate_fdw_ddl(gaia_id_t table_id, const string& server_name)
{
    bool was_transaction_already_active = gaia::db::is_transaction_active();
    if (!was_transaction_already_active)
    {
        gaia::db::begin_transaction();
    }

    stringstream ddl_string_stream;
    ddl_string_stream << "create foreign table ";

    gaia_table_t table = gaia_table_t::get(table_id);
    ddl_string_stream << table.name() << "(" << endl;
    ddl_string_stream << "gaia_id bigint";

    // Concatenate the fields and references.
    vector<gaia_id_t> fields = list_fields(table_id);
    vector<gaia_id_t> references = list_references(table_id);
    fields.insert(fields.end(), references.begin(), references.end());

    for (gaia_id_t field_id : fields)
    {
        gaia_field_t field = gaia_field_t::get(field_id);

        // Skip anonymous reference fields.
        if (field.type() == static_cast<uint8_t>(data_type_t::e_references)
            && strlen(field.name()) == 0)
        {
            continue;
        }

        ddl_string_stream
            << "," << endl
            << generate_fdw_ddl_field(field);
    }

    ddl_string_stream
        << endl
        << ") server " << server_name << ";" << endl;

    if (!was_transaction_already_active)
    {
        gaia::db::commit_transaction();
    }

    return ddl_string_stream.str();
}

string generate_fdw_ddl(
    const string& table_name, const ddl::field_def_list_t& fields, const string& server_name)
{
    stringstream ddl_string_stream;
    ddl_string_stream << "create foreign table ";

    ddl_string_stream << table_name << "(" << endl;
    ddl_string_stream << "gaia_id bigint";

    for (auto& field : fields)
    {
        // Skip anonymous reference fields.
        if (field->type == data_type_t::e_references
            && field->name.length() == 0)
        {
            continue;
        }

        string field_ddl = generate_fdw_ddl_field(field->name, get_fdw_data_type_name(field->type), field->length);
        ddl_string_stream
            << "," << endl
            << field_ddl;
    }

    ddl_string_stream
        << endl
        << ") server " << server_name << ";" << endl;

    return ddl_string_stream.str();
}

} // namespace catalog
} // namespace gaia
