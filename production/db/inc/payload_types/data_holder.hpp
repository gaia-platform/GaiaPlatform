/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <type_traits>

#include "flatbuffers/reflection.h"

namespace gaia
{
namespace db
{
namespace payload_types
{

// Return true if the type is a signed integer type.
inline bool is_signed_integer(reflection::BaseType type)
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

    // Convenience ctors to allow implicit conversion from native types
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

    void clear();

    int compare(const data_holder_t& other) const;
    std::size_t hash() const;

    // Convenience implicit conversions to native types
    operator uint64_t() const;
    operator int64_t() const;
    operator float() const;
    operator double() const;
    operator const char*() const;
};

} // namespace payload_types
} // namespace db
} // namespace gaia
