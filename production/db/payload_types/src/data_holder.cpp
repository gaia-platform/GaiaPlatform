/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "data_holder.hpp"

#include <functional>
#include <string_view>

#include "gaia_internal/common/assert.hpp"

#include "field_access.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace payload_types
{

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

data_holder_t::data_holder_t(const char* value, std::size_t len)
{
    type = reflection::Vector;
    hold.vector_value = {value, len};
}

data_holder_t::operator uint64_t() const
{
    if (!flatbuffers::IsInteger(type) || is_signed_integer(type))
    {
        throw unboxing_error("Unbox failed: type needs to be unsigned integer.");
    }
    auto integer_ptr = reinterpret_cast<const uint64_t*>(&hold.integer_value);
    return *integer_ptr;
}

data_holder_t::operator int64_t() const
{
    if (!is_signed_integer(type))
    {
        throw unboxing_error("Unbox failed: type needs to be signed integer.");
    }
    return hold.integer_value;
}

data_holder_t::operator uint32_t() const
{
    if (!flatbuffers::IsInteger(type) || is_signed_integer(type))
    {
        throw unboxing_error("Unbox failed: type needs to be unsigned integer.");
    }
    auto integer_ptr = reinterpret_cast<const uint64_t*>(&hold.integer_value);
    return static_cast<uint32_t>(*integer_ptr);
}

data_holder_t::operator int32_t() const
{
    if (!is_signed_integer(type))
    {
        throw unboxing_error("Unbox failed: type needs to be signed integer.");
    }
    return static_cast<int32_t>(hold.integer_value);
}

data_holder_t::operator float() const
{
    if (!flatbuffers::IsFloat(type))
    {
        throw unboxing_error("Unbox failed: type needs to be float.");
    }
    return static_cast<float>(hold.float_value);
}

data_holder_t::operator double() const
{
    if (!flatbuffers::IsFloat(type))
    {
        throw unboxing_error("Unbox failed: type needs to be float.");
    }
    return hold.float_value;
}

data_holder_t::operator const char*() const
{
    if (type != reflection::String && type != reflection::Vector)
    {
        throw unboxing_error("Unbox failed: type needs to be string or vector.");
    }

    if (type == reflection::String)
    {
        return hold.string_value;
    }
    return hold.vector_value.data();
}

void data_holder_t::clear()
{
    type = reflection::None;
    hold.integer_value = 0;
}

int data_holder_t::compare(const data_holder_t& other) const
{
    ASSERT_PRECONDITION(type == other.type, "data_holder_t::compare() was called for different types.");

    if (flatbuffers::IsInteger(type))
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
    else if (type == reflection::String)
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
    else if (type == reflection::Vector)
    {
        if (hold.vector_value.data() == nullptr && other.hold.vector_value.data() == nullptr)
        {
            return 0;
        }
        else if (hold.vector_value.data() == nullptr || other.hold.vector_value.data() == nullptr)
        {
            return (hold.vector_value.data() == nullptr) ? -1 : 1;
        }
        else
        {
            // Perform lexicographic compare. This treats vectors as BLOBs, allowing vector
            // to be compared regardless of length.
            //
            // This means APIs using compare() cannot perform introspection, but this provides a path
            // for generic code to handle Vector types.

            size_t len = std::min(other.hold.vector_value.size(), hold.vector_value.size());
            int cmp = memcmp(hold.vector_value.data(), other.hold.vector_value.data(), len);

            if (cmp == 0 && hold.vector_value.size() != other.hold.vector_value.size())
            {
                return (hold.vector_value.size() < other.hold.vector_value.size()) ? -1 : 1;
            }

            return cmp;
        }
    }
    else
    {
        throw unhandled_field_type(type);
    }
}

std::size_t data_holder_t::hash() const
{
    if (flatbuffers::IsInteger(type))
    {
        return std::hash<int64_t>{}(hold.integer_value);
    }
    else if (flatbuffers::IsFloat(type))
    {
        return std::hash<double>{}(hold.float_value);
    }
    else if (type == reflection::String)
    {
        if (hold.string_value == nullptr)
        {
            return 0;
        }
        else
        {
            return std::hash<std::string_view>{}(hold.string_value);
        }
    }
    else if (type == reflection::Vector)
    {
        if (hold.vector_value.data() == nullptr)
        {
            return 0;
        }
        else
        {
            return std::hash<std::string_view>{}(hold.vector_value);
        }
    }
    else
    {
        throw unhandled_field_type(type);
    }
}

std::ostream& operator<<(std::ostream& os, const data_holder_t& data)
{
    if (flatbuffers::IsInteger(data.type))
    {

        if (is_signed_integer(data.type))
        {
            os << data.hold.integer_value;
        }
        else
        {
            os << *(reinterpret_cast<const uint64_t*>(&data.hold.integer_value));
        }
    }
    else if (flatbuffers::IsFloat(data.type))
    {
        os << data.hold.float_value;
    }
    else if (data.type == reflection::String)
    {
        os << data.hold.string_value;
    }
    else if (data.type == reflection::Vector)
    {
        os << "<vector>";
    }
    else
    {
        throw unhandled_field_type(data.type);
    }

    return os;
}

} // namespace payload_types
} // namespace db
} // namespace gaia
