/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <type_holder.hpp>

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
