/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

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

struct vector_value_t
{
    const uint8_t* data;
    uint32_t size;
};

// This union allows us to reference values of different types.
// We do not manage the string_value memory. The caller is responsible for it.
union value_holder_t
{
    int64_t integer_value;
    double float_value;
    const char* string_value;
    vector_value_t vector_value;
};

// A simple structure that isolates us from the details
// of the encapsulated data type.
struct data_holder_t
{
    reflection::BaseType type;
    value_holder_t hold;

    data_holder_t();

    void clear();

    int compare(const data_holder_t& other) const;
};

} // namespace payload_types
} // namespace db
} // namespace gaia
