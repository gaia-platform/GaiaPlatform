/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "data_holder.hpp"

#include <functional>

#include "field_access.hpp"
#include "retail_assert.hpp"

using namespace gaia::common;
using namespace gaia::db::payload_types;

data_holder_t::data_holder_t()
{
    clear();
}

data_holder_t::data_holder_t(float value)
{
    type = reflection::Float;
    hold.float_value = value;
}

data_holder_t::data_holder_t(double value)
{
    type = reflection::Double;
    hold.float_value = value;
}

data_holder_t::data_holder_t(const char* value)
{
    type = reflection::String;
    hold.string_value = value;
}

data_holder_t::operator uint64_t() const
{
    retail_assert(flatbuffers::IsInteger(type) && !is_signed_integer(type), "Unbox failed: type needs to be unsigned integer.");
    auto integer_ptr = reinterpret_cast<const uint64_t*>(&hold.integer_value);
    return *integer_ptr;
}

data_holder_t::operator int64_t() const
{
    retail_assert(is_signed_integer(type), "Unbox failed: type needs to be signed integer.");
    return hold.integer_value;
}

data_holder_t::operator float() const
{
    retail_assert(flatbuffers::IsFloat(type), "Unbox failed: type needs to be float.");
    auto float_ptr = reinterpret_cast<const float*>(&hold.float_value);
    return *float_ptr;
}

data_holder_t::operator double() const
{
    retail_assert(flatbuffers::IsFloat(type), "Unbox failed: type needs to be double.");
    return hold.float_value;
}

data_holder_t::operator const char*() const
{
    retail_assert(type == reflection::String, "Unbox failed: type needs to be String.");
    return hold.string_value;
}

void data_holder_t::clear()
{
    type = reflection::None;
    hold.integer_value = 0;
}

int data_holder_t::compare(const data_holder_t& other) const
{
    retail_assert(type == other.type, "type_holder_t::compare() was called for different types.");

    if (type == reflection::String)
    {
        if (hold.string_value == nullptr && other.hold.string_value == nullptr)
        {
            return 0;
        }
        else if (hold.string_value == nullptr || other.hold.string_value == nullptr)
        {
            return (hold.string_value == nullptr) ? -1 : 1;
        }
        else
        {
            return strcmp(hold.string_value, other.hold.string_value);
        }
    }
    else if (flatbuffers::IsInteger(type))
    {
        if (is_signed_integer(type))
        {
            return (hold.integer_value == other.hold.integer_value)
                ? 0
                : (hold.integer_value > other.hold.integer_value) ? 1 : -1;
        }
        else
        {
            if (hold.integer_value == other.hold.integer_value)
            {
                return 0;
            }

            auto unsigned_integer_value = reinterpret_cast<const uint64_t*>(&hold.integer_value);
            auto other_unsigned_integer_value = reinterpret_cast<const uint64_t*>(&other.hold.integer_value);

            return (*unsigned_integer_value > *other_unsigned_integer_value) ? 1 : -1;
        }
    }
    else if (flatbuffers::IsFloat(type))
    {
        return (hold.float_value == other.hold.float_value)
            ? 0
            : (hold.float_value > other.hold.float_value) ? 1 : -1;
    }
    else
    {
        throw unhandled_field_type(type);
    }
}

std::size_t data_holder_t::hash() const
{
    if (type == reflection::String)
    {
        if (hold.string_value == nullptr)
        {
            return 0;
        }
        else
        {
            return std::hash<std::string>{}(hold.string_value);
        }
    }
    else if (flatbuffers::IsInteger(type))
    {
        return std::hash<int64_t>{}(hold.integer_value);
    }
    else if (flatbuffers::IsFloat(type))
    {
        return std::hash<double>{}(hold.float_value);
    }
    else
    {
        throw unhandled_field_type(type);
    }
}
