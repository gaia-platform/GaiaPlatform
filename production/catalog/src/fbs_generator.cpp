////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "fbs_generator.hpp"

#include <algorithm>
#include <sstream>
#include <string>

#include <flatbuffers/idl.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/exceptions.hpp"

using namespace gaia::common;

using std::string;
using std::stringstream;
using std::to_string;

namespace gaia
{
namespace catalog
{

namespace
{

string generate_fbs_namespace(const string& db_name)
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

string generate_fbs_field(const gaia_field_t& field, bool ignore_optional = false)
{
    string name{field.name()};
    auto type = static_cast<data_type_t>(field.type());
    string type_name{get_data_type_name(type)};
    uint16_t repeated_count = field.repeated_count();

    std::stringstream ss;
    ss << name;

    if (repeated_count == 1)
    {
        ss << ":" + type_name;
    }
    else if (repeated_count == 0)
    {
        ss << ":[" + type_name + "] (required)";
    }
    else
    {
        ss << ":[" + type_name + ":" + to_string(repeated_count) + "]";
    }

    if (!ignore_optional && field.optional() && type != data_type_t::e_string && repeated_count == 1)
    {
        ss << "=null";
    }

    return ss.str();
}

} // namespace

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
        ASSERT_UNREACHABLE(message.str().c_str());
    }
}

string generate_fbs(gaia_id_t table_id, bool ignore_optional)
{
    string fbs;
    gaia_table_t table = gaia_table_t::get(table_id);
    fbs += generate_fbs_namespace(table.database().name());
    string table_name{table.name()};
    fbs += "table " + table_name + "{\n";
    for (gaia_id_t field_id : list_fields(table_id))
    {
        gaia_field_t field = gaia_field_t::get(field_id);
        fbs += "\t" + generate_fbs_field(field, ignore_optional) + ";\n";
    }
    fbs += "}\n";
    fbs += "root_type " + table_name + ";";
    return fbs;
}

string generate_fbs(const string& db_name)
{
    gaia_id_t db_id = find_db_id(db_name);
    if (db_id == c_invalid_gaia_id)
    {
        throw db_does_not_exist_internal(db_name);
    }
    string fbs = generate_fbs_namespace(db_name);
    gaia::db::begin_transaction();
    for (auto const& table : gaia_database_t::get(db_id).gaia_tables())
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

std::vector<uint8_t> generate_bfbs(const string& fbs)
{
    flatbuffers::Parser fbs_parser;
    bool parsing_result = fbs_parser.Parse(fbs.c_str());
    ASSERT_PRECONDITION(parsing_result == true, "Invalid FlatBuffers schema!");
    fbs_parser.Serialize();

    // Use the std::vector (begin, end) iterator constructor.
    return {
        fbs_parser.builder_.GetBufferPointer(),
        fbs_parser.builder_.GetBufferPointer() + fbs_parser.builder_.GetSize()};
}

} // namespace catalog
} // namespace gaia
