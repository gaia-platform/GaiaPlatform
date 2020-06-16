/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "flatbuffers/reflection.h"

namespace gaia
{
namespace db
{
namespace types
{

inline bool is_signed_integer(reflection::BaseType t)
{
    // Signed integer types have odd BaseType enum values.
    return flatbuffers::IsInteger(t) && (((int)t) % 2 == 1);
}

struct type_holder_t
{
    reflection::BaseType type;

    int64_t integer_value;
    double float_value;
    const char* string_value;

    type_holder_t();

    void clear();

    int compare(const type_holder_t& other) const;
};

}
}
}
