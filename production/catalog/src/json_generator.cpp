/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "json_generator.hpp"

#include <algorithm>
#include <sstream>
#include <string>

#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

#include "gaia_catalog.h"
#include "retail_assert.hpp"

namespace gaia
{
namespace catalog
{

constexpr char c_default_bool_value[] = "false";
constexpr char c_default_integer_value[] = "0";
constexpr char c_default_float_value[] = "0.0";
constexpr char c_default_string_value[] = "\"\"";

static string generate_json_field(const string& name, const string& default_type_value, int count)
{
    if (count == 1)
    {
        return name + ":" + default_type_value;
    }
    else if (count == 0)
    {
        return name + ":[" + default_type_value + "]";
    }
    else
    {
        stringstream string_stream;
        string_stream << name << ":[" << default_type_value;

        for (int i = 1; i < count; ++i)
        {
            string_stream << ", " << default_type_value;
        }
        string_stream << "]";

        return string_stream.str();
    }
}

string get_data_type_default_value(data_type_t data_type)
{
    switch (data_type)
    {
    case data_type_t::e_bool:
        return c_default_bool_value;

    case data_type_t::e_int8:
    case data_type_t::e_uint8:
    case data_type_t::e_int16:
    case data_type_t::e_uint16:
    case data_type_t::e_int32:
    case data_type_t::e_uint32:
    case data_type_t::e_int64:
    case data_type_t::e_uint64:
        return c_default_integer_value;

    case data_type_t::e_float:
    case data_type_t::e_double:
        return c_default_float_value;

    case data_type_t::e_string:
        return c_default_string_value;

    default:
        stringstream message;
        message
            << "Unhandled data_type_t value " << static_cast<int>(data_type)
            << " in get_data_type_default_value()!";
        // If we use retail_assert(false), the compiler can't figure out
        // that it will throw an exception and will warn us about
        // potentially exiting the method without returning a value.
        throw retail_assertion_failure(message.str());
    }
}

static string generate_json_field(const gaia_field_t& field)
{
    string name(field.name());
    string default_type_value = get_data_type_default_value(static_cast<data_type_t>(field.type()));
    return generate_json_field(name, default_type_value, field.repeated_count());
}

string generate_json(gaia_id_t table_id)
{
    gaia::db::begin_transaction();

    stringstream json_string_stream;
    json_string_stream << "{";

    gaia_table_t table = gaia_table_t::get(table_id);
    bool has_output_first_field = false;
    for (gaia_id_t field_id : list_fields(table_id))
    {
        if (has_output_first_field)
        {
            json_string_stream << ",";
        }
        has_output_first_field = true;

        gaia_field_t field = gaia_field_t::get(field_id);
        json_string_stream
            << endl
            << generate_json_field(field);
    }

    json_string_stream
        << endl
        << "}" << endl;

    gaia::db::commit_transaction();

    return json_string_stream.str();
}

string generate_json(const ddl::field_def_list_t& fields)
{
    stringstream json_string_stream;
    json_string_stream << "{";

    bool has_output_first_field = false;
    for (auto& field : fields)
    {
        if (field->type == data_type_t::e_references)
        {
            continue;
        }

        if (has_output_first_field)
        {
            json_string_stream << ",";
        }
        has_output_first_field = true;

        string field_json = generate_json_field(field->name, get_data_type_default_value(field->type), field->length);

        json_string_stream
            << endl
            << field_json;
    }

    json_string_stream
        << endl
        << "}" << endl;

    return json_string_stream.str();
}

string generate_bin(const string& fbs, const string& json)
{
    flatbuffers::IDLOptions options;
    options.force_defaults = true;
    flatbuffers::Parser parser(options);

    bool parsing_result = parser.Parse(fbs.c_str());
    retail_assert(parsing_result == true, "Invalid FlatBuffers schema!");

    parsing_result = parser.Parse(json.c_str());
    retail_assert(parsing_result == true, "Invalid FlatBuffers JSON!");

    // Use the fbs method ""flatbuffers::BufferToHexText" to encode the buffer.
    // Some encoding is needed to store the binary as string in fbs payload
    // because fbs assumes strings are null terminated whereas the
    // serialization templates may have null characters in them.
    //
    // The following const defines the line wrap length of the encoded hex text.
    // We do not need this but fbs method requires it.
    constexpr size_t c_encoding_hex_text_len = 80;
    return flatbuffers::BufferToHexText(
        parser.builder_.GetBufferPointer(), parser.builder_.GetSize(),
        c_encoding_hex_text_len, "", "");
}

string get_bin(gaia_id_t table_id)
{
    string serialization_template;
    gaia_table_t table = gaia_table_t::get(table_id);

    // The delimitation character used by the hex encoding method.
    constexpr char c_hex_text_delim = ',';
    const char* p = table.serialization_template();
    while (*p != '\0')
    {
        if (*p == '\n')
        {
            p++;
            continue;
        }
        else if (*p == c_hex_text_delim)
        {
            p++;
            continue;
        }
        else
        {
            char* endptr;
            unsigned byte = std::strtoul(p, &endptr, 0);
            retail_assert(endptr != p && errno != ERANGE, "Invalid hex template serialization");
            serialization_template.push_back(byte);
            p = endptr;
        }
    }

    return serialization_template;
}

} // namespace catalog
} // namespace gaia
