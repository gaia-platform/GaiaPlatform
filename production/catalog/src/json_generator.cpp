/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "json_generator.hpp"

#include <algorithm>
#include <sstream>
#include <string>

#include <flatbuffers/idl.h>

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/assert.hpp"

#include "gaia_spdlog/fmt/fmt.h"

using namespace std;
using namespace gaia::common;

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
            << "Unhandled data_type_t value '" << static_cast<int>(data_type)
            << "' in get_data_type_default_value()!";
        ASSERT_UNREACHABLE(message.str().c_str());
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
    stringstream json_string_stream;
    json_string_stream << "{";

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

    return json_string_stream.str();
}

vector<uint8_t> generate_bin(const string& fbs, const string& json)
{
    flatbuffers::IDLOptions options;
    options.force_defaults = true;
    flatbuffers::Parser parser(options);

    bool parsing_result = parser.Parse(fbs.c_str());
    ASSERT_INVARIANT(
        parsing_result == true, gaia_fmt::format("Invalid FlatBuffers schema: {}", parser.error_).c_str());

    parsing_result = parser.Parse(json.c_str());
    ASSERT_INVARIANT(
        parsing_result == true, gaia_fmt::format("Invalid FlatBuffers JSON: {}", parser.error_).c_str());

    // Use the std::vector (begin, end) iterator constructor.
    return {
        parser.builder_.GetBufferPointer(),
        parser.builder_.GetBufferPointer() + parser.builder_.GetSize()};
}

} // namespace catalog
} // namespace gaia
