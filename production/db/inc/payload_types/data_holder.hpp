/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <ostream>
#include <string_view>
#include <type_traits>

#include "flatbuffers/reflection.h"

#include "gaia/exception.hpp"

namespace gaia
{
namespace db
{
namespace payload_types
{

// We are using std::size_t for no other reason than std::hash() does the same.
typedef std::size_t data_hash_t;

class unboxing_error : public gaia::common::gaia_exception
{
public:
    explicit unboxing_error(const std::string& message)
    {
        m_message = message;
    }
};

// Return true if the type is a signed integer type.
inline bool
is_signed_integer(reflection::BaseType type)
{
    // Signed integer types have odd BaseType enum values.
    // This logic can break if flatbuffers adds support
    // for new larger integers and doesn't maintain this property,
    // but any check would share this weakness
    // and this approach is quicker than others.
    return flatbuffers::IsInteger(type) && (((int)type) % 2 == 1);
}

// This union allows us to reference values of different types.
// We do not manage the string_value memory. The caller is responsible for it.
union value_holder_t
{
    int64_t integer_value;
    double float_value;
    const char* string_value;
    std::string_view vector_value{};
};

// A simple structure that isolates us from the details
// of the encapsulated data type.
struct data_holder_t
{
    reflection::BaseType type;
    value_holder_t hold;

    data_holder_t();
    data_holder_t(const data_holder_t&) = default;
    data_holder_t(data_holder_t&&) = default;

    // Convenience ctors to allow implicit conversion from native types.
    template <typename T>
    data_holder_t(T value, typename std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>* = nullptr)
    {
        type = reflection::Int;
        hold.integer_value = value;
    }
    template <typename T>
    data_holder_t(T value, typename std::enable_if_t<std::is_integral_v<T> && !std::is_signed_v<T>>* = nullptr)
    {
        type = reflection::UInt;
        hold.integer_value = value;
    }
    data_holder_t(float value);
    data_holder_t(double value);
    data_holder_t(const char* value);
    data_holder_t(const char* value, size_t len);

    void clear();

    int compare(const data_holder_t& other) const;
    data_hash_t hash() const;

    // Convenience implicit conversions to native types.
    operator int32_t() const;
    operator uint32_t() const;
    operator uint64_t() const;
    operator int64_t() const;
    operator float() const;
    operator double() const;
    operator const char*() const;

    // For pretty-print/debugging.
    friend std::ostream& operator<<(std::ostream& os, const data_holder_t& data);
};

} // namespace payload_types
} // namespace db
} // namespace gaia
