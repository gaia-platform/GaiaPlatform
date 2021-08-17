/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "reflection.hpp"

#include <sstream>

namespace gaia
{
namespace db
{

const char* data_type_to_string(common::data_type_t type)
{
    switch (type)
    {
    case common::data_type_t::e_bool:
        return "e_bool";
    case common::data_type_t::e_uint8:
        return "e_uint8";
    case common::data_type_t::e_int8:
        return "e_int8";
    case common::data_type_t::e_uint16:
        return "e_uint16";
    case common::data_type_t::e_int16:
        return "e_int16";
    case common::data_type_t::e_uint32:
        return "e_uint32";
    case common::data_type_t::e_int32:
        return "e_int32";
    case common::data_type_t::e_uint64:
        return "e_uint64";
    case common::data_type_t::e_int64:
        return "e_int64";
    case common::data_type_t::e_float:
        return "e_float";
    case common::data_type_t::e_double:
        return "e_double";
    case common::data_type_t::e_string:
        return "e_string";
    default:
        return "unknown";
    }
}

data_type_not_handled_t::data_type_not_handled_t(common::data_type_t type)
{
    std::stringstream string_stream;
    string_stream << "Data type not handled: " << data_type_to_string(type) << ".";
    m_message = string_stream.str();
}

} // namespace db
} // namespace gaia
