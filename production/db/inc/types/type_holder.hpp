/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <unordered_map>

#include "flatbuffers/reflection.h"

#include <synchronization.hpp>

namespace gaia
{
namespace db
{
namespace types
{

struct type_holder_t
{
    reflection::BaseType type;

    int64_t integer_value;
    double float_value;
    const char* string_value;

    type_holder_t();

    void clear();
};

}
}
}
