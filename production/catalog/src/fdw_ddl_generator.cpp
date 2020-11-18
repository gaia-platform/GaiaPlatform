/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <sstream>
#include <string>

#include "gaia/retail_assert.hpp"

#include "catalog_internal.hpp"
#include "gaia_catalog.h"

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
        return name + " BYTEA";
    }
}

string get_fdw_data_type_name(data_type_t data_type)
{
    switch (data_type)
    {
    case data_type_t::e_bool:
        return "BOOLEAN";

    case data_type_t::e_int8:
    case data_type_t::e_uint8:
    case data_type_t::e_int16:
    case data_type_t::e_uint16:
        return "SMALLINT";

    case data_type_t::e_int32:
    case data_type_t::e_uint32:
        return "INTEGER";

    case data_type_t::e_int64:
    case data_type_t::e_uint64:
        return "BIGINT";

    case data_type_t::e_float:
        return "REAL";

    case data_type_t::e_double:
        return "DOUBLE PRECISION";

    case data_type_t::e_string:
        return "TEXT";

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
    ddl_string_stream << "CREATE FOREIGN TABLE ";

    gaia_table_t table = gaia_table_t::get(table_id);
    ddl_string_stream << table.name() << "(" << endl;
    ddl_string_stream << "gaia_id BIGINT";

    for (gaia_id_t field_id : list_fields(table_id))
    {
        gaia_field_t field = gaia_field_t::get(field_id);

        ddl_string_stream
            << "," << endl
            << generate_fdw_ddl_field(field);
    }

    for (gaia_id_t reference_id : list_references(table_id))
    {
        gaia_relationship_t relationship = gaia_relationship_t::get(reference_id);

        string relationship_name = relationship.name();
        if (relationship_name.empty())
        {
            relationship_name = relationship.parent_gaia_table().name();
        }
        retail_assert(
            !relationship_name.empty(),
            "Unable to derive name of anonymous relationship!");

        ddl_string_stream
            << "," << endl
            << relationship_name << " BIGINT";
    }

    ddl_string_stream
        << endl
        << ") SERVER " << server_name << ";" << endl;

    if (!was_transaction_already_active)
    {
        gaia::db::commit_transaction();
    }

    return ddl_string_stream.str();
}

} // namespace catalog
} // namespace gaia
