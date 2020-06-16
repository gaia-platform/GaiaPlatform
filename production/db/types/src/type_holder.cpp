/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <type_holder.hpp>

#include <retail_assert.hpp>
#include <field_access.hpp>

using namespace gaia::common;
using namespace gaia::db::types;

type_holder_t::type_holder_t()
{
    clear();
}

void type_holder_t::clear()
{
    type = reflection::None;

    integer_value = 0;
    float_value = 0;
    string_value = nullptr;
}

int type_holder_t::compare(const type_holder_t& other) const
{
    retail_assert(type == other.type, "type_holder_t::compare() was called for different types.");

    if (type == reflection::String)
    {
        if (string_value == nullptr && other.string_value == nullptr)
        {
            return 0;
        }
        else if (string_value == nullptr || other.string_value == nullptr)
        {
            return string_value == nullptr ? -1 : 1;
        }
        else
        {
            return strcmp(string_value, other.string_value);
        }
    }
    else if (flatbuffers::IsInteger(type))
    {
        if (is_signed_integer(type))
        {
            return (integer_value == other.integer_value)
                ? 0
                : (integer_value > other.integer_value) ? 1 : -1;
        }
        else
        {
            if (integer_value == other.integer_value)
            {
                return 0;
            }

            const uint64_t* unsigned_integer_value = reinterpret_cast<const uint64_t*>(&integer_value);
            const uint64_t* other_unsigned_integer_value = reinterpret_cast<const uint64_t*>(&other.integer_value);

            return (*unsigned_integer_value > *other_unsigned_integer_value) ? 1 : -1;
        }
    }
    else if (flatbuffers::IsFloat(type))
    {
        return (float_value == other.float_value)
            ? 0
            : (float_value > other.float_value) ? 1 : -1;
    }
    else
    {
        throw unhandled_field_type(type);
    }
}
