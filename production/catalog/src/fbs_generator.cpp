/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "fbs_generator.hpp"

#include <algorithm>
#include <sstream>
#include <string>

#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

#include "flatbuffers_helpers.hpp"
#include "gaia_catalog.h"
#include "retail_assert.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace catalog
{

/**
 * Helper functions
 **/

static string generate_fbs_namespace(const string& db_name)
{
    if (db_name.empty() || db_name == c_empty_db_name)
    {
        return "namespace " + c_gaia_namespace + "." + c_internal_suffix + ";\n";
    }
    else
    {
        return "namespace " + c_gaia_namespace + "." + db_name + "." + c_internal_suffix + ";\n";
    }
}

static string generate_fbs_field(const string& name, const string& type, int count)
{
    if (count == 1)
    {
        return name + ":" + type;
    }
    else if (count == 0)
    {
        return name + ":[" + type + "]";
    }
    else
    {
        return name + ":[" + type + ":" + to_string(count) + "]";
    }
}

static string generate_fbs_field(const gaia_field_t& field)
{
    string name{field.name()};
    string type{get_data_type_name(static_cast<data_type_t>(field.type()))};
    return generate_fbs_field(name, type, field.repeated_count());
}

string get_data_type_name(data_type_t data_type)
{
    switch (data_type)
    {
    case data_type_t::e_bool:
        return "bool";
    case data_type_t::e_int8:
        return "int8";
    case data_type_t::e_uint8:
        return "uint8";
    case data_type_t::e_int16:
        return "int16";
    case data_type_t::e_uint16:
        return "uint16";
    case data_type_t::e_int32:
        return "int32";
    case data_type_t::e_uint32:
        return "uint32";
    case data_type_t::e_int64:
        return "int64";
    case data_type_t::e_uint64:
        return "uint64";
    case data_type_t::e_float:
        return "float";
    case data_type_t::e_double:
        return "double";
    case data_type_t::e_string:
        return "string";
    default:
        stringstream message;
        message
            << "Unhandled data_type_t value '" << static_cast<int>(data_type)
            << "' in get_data_type_name()!";
        // If we use retail_assert(false), the compiler can't figure out
        // that it will throw an exception and will warn us about
        // potentially exiting the method without returning a value.
        throw retail_assertion_failure(message.str());
    }
}

string generate_fbs(gaia_id_t table_id)
{
    string fbs;
    gaia::db::begin_transaction();
    gaia_table_t table = gaia_table_t::get(table_id);
    fbs += generate_fbs_namespace(table.gaia_database().name());
    string table_name{table.name()};
    fbs += "table " + table_name + "{\n";
    for (gaia_id_t field_id : list_fields(table_id))
    {
        gaia_field_t field = gaia_field_t::get(field_id);
        fbs += "\t" + generate_fbs_field(field) + ";\n";
    }
    fbs += "}\n";
    fbs += "root_type " + table_name + ";";
    gaia::db::commit_transaction();
    return fbs;
}

string generate_fbs(const string& dbname)
{
    gaia_id_t db_id = find_db_id(dbname);
    if (db_id == c_invalid_gaia_id)
    {
        throw db_not_exists(dbname);
    }
    string fbs = generate_fbs_namespace(dbname);
    gaia::db::begin_transaction();
    for (auto const& table : gaia_database_t::get(db_id).gaia_table_list())
    {
        fbs += "table " + string(table.name()) + "{\n";
        for (gaia_id_t field_id : list_fields(table.gaia_id()))
        {
            gaia_field_t field = gaia_field_t::get(field_id);
            fbs += "\t" + generate_fbs_field(field) + ";\n";
        }
        fbs += "}\n\n";
    }
    gaia::db::commit_transaction();
    return fbs;
}

string generate_fbs(const string& db_name, const string& table_name, const ddl::field_def_list_t& fields)
{
    string fbs = generate_fbs_namespace(db_name);
    fbs += "table " + table_name + "{";
    for (auto& field : fields)
    {
        if (field->field_type == ddl::field_type_t::reference)
        {
            continue;
        }
        const ddl::data_field_def_t* data_field = dynamic_cast<ddl::data_field_def_t*>(field.get());
        string field_fbs = generate_fbs_field(
            data_field->name, get_data_type_name(data_field->data_type), data_field->length);
        fbs += field_fbs + ";";
    }
    fbs += "}";
    fbs += "root_type " + table_name + ";";
    return fbs;
}

string generate_bfbs(const string& fbs)
{
    flatbuffers::Parser fbs_parser;
    bool parsing_result = fbs_parser.Parse(fbs.c_str());
    retail_assert(parsing_result == true, "Invalid FlatBuffers schema!");
    fbs_parser.Serialize();
    // Use the fbs method ""flatbuffers::BufferToHexText" to encode the buffer.
    // Some encoding is needed to store the binary as string in fbs payload
    // because fbs assumes strings are null terminated while the binary schema
    // buffers may have null characters in them.
    //
    // The following const defines the line wrap length of the encoded hex text.
    // We do not need this but fbs method requires it.
    constexpr size_t c_binary_schema_hex_text_len = 80;
    return flatbuffers::BufferToHexText(
        fbs_parser.builder_.GetBufferPointer(), fbs_parser.builder_.GetSize(), c_binary_schema_hex_text_len, "", "");
}

vector<uint8_t> get_bfbs(gaia_id_t table_id)
{
    gaia_table_t table = gaia_table_t::get(table_id);
    return gaia::common::flatbuffers_hex_to_buffer(table.binary_schema());
}

} // namespace catalog
} // namespace gaia
