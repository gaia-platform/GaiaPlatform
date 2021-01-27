/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "data_holder.hpp"

#include "field_access.hpp"

#include "gaia_internal/common/retail_assert.hpp"

using namespace gaia::common;
using namespace gaia::db::payload_types;

data_holder_t::data_holder_t()
{
    clear();
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
