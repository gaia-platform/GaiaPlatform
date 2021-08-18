/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "flatbuffers/reflection.h"

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

namespace gaia
{
namespace db
{

class data_type_not_handled_t : public gaia::common::gaia_exception
{
public:
    explicit data_type_not_handled_t(common::data_type_t type);
};

inline reflection::BaseType
convert_to_reflection_type(common::data_type_t type)
{
    switch (type)
    {
    case common::data_type_t::e_bool:
        return reflection::Bool;
    case common::data_type_t::e_uint8:
        return reflection::UByte;
    case common::data_type_t::e_int8:
        return reflection::Byte;
    case common::data_type_t::e_uint16:
        return reflection::UShort;
    case common::data_type_t::e_int16:
        return reflection::Short;
    case common::data_type_t::e_uint32:
        return reflection::UInt;
    case common::data_type_t::e_int32:
        return reflection::Int;
    case common::data_type_t::e_uint64:
        return reflection::ULong;
    case common::data_type_t::e_int64:
        return reflection::Long;
    case common::data_type_t::e_float:
        return reflection::Float;
    case common::data_type_t::e_double:
        return reflection::Double;
    case common::data_type_t::e_string:
        return reflection::String;
    default:
        throw data_type_not_handled_t(type);
    }
}

} // namespace db
} // namespace gaia
