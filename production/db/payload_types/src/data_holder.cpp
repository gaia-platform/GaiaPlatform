/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "data_holder.hpp"

#include <functional>
#include <string_view>

#include "gaia_internal/common/retail_assert.hpp"

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
    m_type = reflection::Float;
    m_hold.float_value = value;
}

data_holder_t::data_holder_t(double value)
{
    m_type = reflection::Double;
    m_hold.float_value = value;
}

data_holder_t::data_holder_t(const char* value)
{
    m_type = reflection::String;
    m_hold.string_value = value;
}

data_holder_t::data_holder_t(const char* value, std::size_t len)
{
    m_type = reflection::Vector;
    m_hold.vector_value = {value, len};
}

data_holder_t::data_holder_t(data_read_buffer_t& buffer, reflection::BaseType type)
{
    m_type = type;

    if (flatbuffers::IsInteger(m_type))
    {
        buffer >> m_hold.integer_value;
    }
    else if (flatbuffers::IsFloat(m_type))
    {
        buffer >> m_hold.float_value;
    }
    else if (type == reflection::String)
    {
        size_t length;
        buffer >> length;

        if (length == 0)
        {
            m_hold.string_value = nullptr;
        }
        else
        {
            m_hold.string_value = buffer.read(length);
        }
    }
    else if (m_type == reflection::Vector)
    {
        size_t length;
        buffer >> length;

        if (length == 0)
        {
            m_hold.vector_value = {nullptr, 0};
        }
        else
        {
            m_hold.vector_value = {buffer.read(length), length};
        }
    }
    else
    {
        throw unhandled_field_type(m_type);
    }
}

data_holder_t::operator uint64_t() const
{
    if (!flatbuffers::IsInteger(m_type) || is_signed_integer(m_type))
    {
        throw unboxing_error("Unbox failed: type needs to be unsigned integer.");
    }
    auto integer_ptr = reinterpret_cast<const uint64_t*>(&m_hold.integer_value);
    return *integer_ptr;
}

data_holder_t::operator int64_t() const
{
    if (!is_signed_integer(m_type))
    {
        throw unboxing_error("Unbox failed: type needs to be signed integer.");
    }
    return m_hold.integer_value;
}

data_holder_t::operator uint32_t() const
{
    if (!flatbuffers::IsInteger(m_type) || is_signed_integer(m_type))
    {
        throw unboxing_error("Unbox failed: type needs to be unsigned integer.");
    }
    auto integer_ptr = reinterpret_cast<const uint64_t*>(&m_hold.integer_value);
    return static_cast<uint32_t>(*integer_ptr);
}

data_holder_t::operator int32_t() const
{
    if (!is_signed_integer(m_type))
    {
        throw unboxing_error("Unbox failed: type needs to be signed integer.");
    }
    return static_cast<int32_t>(m_hold.integer_value);
}

data_holder_t::operator float() const
{
    if (!flatbuffers::IsFloat(m_type))
    {
        throw unboxing_error("Unbox failed: type needs to be float.");
    }
    return static_cast<float>(m_hold.float_value);
}

data_holder_t::operator double() const
{
    if (!flatbuffers::IsFloat(m_type))
    {
        throw unboxing_error("Unbox failed: type needs to be float.");
    }
    return m_hold.float_value;
}

data_holder_t::operator const char*() const
{
    if (m_type != reflection::String && m_type != reflection::Vector)
    {
        throw unboxing_error("Unbox failed: type needs to be string or vector.");
    }

    if (m_type == reflection::String)
    {
        return m_hold.string_value;
    }
    return m_hold.vector_value.data();
}

void data_holder_t::clear()
{
    m_type = reflection::None;
    m_hold.integer_value = 0;
}

int data_holder_t::compare(const data_holder_t& other) const
{
    ASSERT_PRECONDITION(m_type == other.m_type, "data_holder_t::compare() was called for different types.");

    if (flatbuffers::IsInteger(m_type))
    {
        if (is_signed_integer(m_type))
        {
            return (m_hold.integer_value == other.m_hold.integer_value)
                ? 0
                : (m_hold.integer_value > other.m_hold.integer_value) ? 1 : -1;
        }
        else
        {
            if (m_hold.integer_value == other.m_hold.integer_value)
            {
                return 0;
            }

            auto unsigned_integer_value = reinterpret_cast<const uint64_t*>(&m_hold.integer_value);
            auto other_unsigned_integer_value = reinterpret_cast<const uint64_t*>(&other.m_hold.integer_value);

            return (*unsigned_integer_value > *other_unsigned_integer_value) ? 1 : -1;
        }
    }
    else if (flatbuffers::IsFloat(m_type))
    {
        return (m_hold.float_value == other.m_hold.float_value)
            ? 0
            : (m_hold.float_value > other.m_hold.float_value) ? 1 : -1;
    }
    else if (m_type == reflection::String)
    {
        if (m_hold.string_value == nullptr && other.m_hold.string_value == nullptr)
        {
            return 0;
        }
        else if (m_hold.string_value == nullptr || other.m_hold.string_value == nullptr)
        {
            return (m_hold.string_value == nullptr) ? -1 : 1;
        }
        else
        {
            return strcmp(m_hold.string_value, other.m_hold.string_value);
        }
    }
    else if (m_type == reflection::Vector)
    {
        if (m_hold.vector_value.data() == nullptr && other.m_hold.vector_value.data() == nullptr)
        {
            return 0;
        }
        else if (m_hold.vector_value.data() == nullptr || other.m_hold.vector_value.data() == nullptr)
        {
            return (m_hold.vector_value.data() == nullptr) ? -1 : 1;
        }
        else
        {
            // Perform lexicographic compare. This treats vectors as BLOBs, allowing vector
            // to be compared regardless of length.
            //
            // This means APIs using compare() cannot perform introspection, but this provides a path
            // for generic code to handle Vector types.

            size_t len = std::min(other.m_hold.vector_value.size(), m_hold.vector_value.size());
            int cmp = memcmp(m_hold.vector_value.data(), other.m_hold.vector_value.data(), len);

            if (cmp == 0 && m_hold.vector_value.size() != other.m_hold.vector_value.size())
            {
                return (m_hold.vector_value.size() < other.m_hold.vector_value.size()) ? -1 : 1;
            }

            return cmp;
        }
    }
    else
    {
        throw unhandled_field_type(m_type);
    }
}

std::size_t data_holder_t::hash() const
{
    if (flatbuffers::IsInteger(m_type))
    {
        return std::hash<int64_t>{}(m_hold.integer_value);
    }
    else if (flatbuffers::IsFloat(m_type))
    {
        return std::hash<double>{}(m_hold.float_value);
    }
    else if (m_type == reflection::String)
    {
        if (m_hold.string_value == nullptr)
        {
            return 0;
        }
        else
        {
            return std::hash<std::string_view>{}(m_hold.string_value);
        }
    }
    else if (m_type == reflection::Vector)
    {
        if (m_hold.vector_value.data() == nullptr)
        {
            return 0;
        }
        else
        {
            return std::hash<std::string_view>{}(m_hold.vector_value);
        }
    }
    else
    {
        throw unhandled_field_type(m_type);
    }
}

void data_holder_t::serialize(data_write_buffer_t& buffer) const
{
    if (flatbuffers::IsInteger(m_type))
    {
        buffer << m_hold.integer_value;
    }
    else if (flatbuffers::IsFloat(m_type))
    {
        buffer << m_hold.float_value;
    }
    else if (m_type == reflection::String)
    {
        size_t length;
        if (m_hold.string_value == nullptr)
        {
            length = 0;
            buffer << length;
        }
        else
        {
            length = strlen(m_hold.string_value) + 1; // +1 for null terminator
            buffer << length;
            buffer.write(m_hold.string_value, length);
        }
    }
    else if (m_type == reflection::Vector)
    {
        size_t length;

        if (m_hold.vector_value.data() == nullptr)
        {
            length = 0;
            buffer << length;
        }
        else
        {
            length = m_hold.vector_value.size();
            buffer << length;
            buffer.write(m_hold.vector_value.data(), length);
        }
    }
    else
    {
        throw unhandled_field_type(m_type);
    }
}

std::ostream& operator<<(std::ostream& os, const data_holder_t& data)
{
    if (flatbuffers::IsInteger(data.m_type))
    {
        if (is_signed_integer(data.m_type))
        {
            os << data.m_hold.integer_value;
        }
        else
        {
            os << *(reinterpret_cast<const uint64_t*>(&data.m_hold.integer_value));
        }
    }
    else if (flatbuffers::IsFloat(data.m_type))
    {
        os << data.m_hold.float_value;
    }
    else if (data.m_type == reflection::String)
    {
        os << data.m_hold.string_value;
    }
    else if (data.m_type == reflection::Vector)
    {
        os << "<vector>";
    }
    else
    {
        throw unhandled_field_type(data.m_type);
    }

    return os;
}

} // namespace payload_types
} // namespace db
} // namespace gaia
