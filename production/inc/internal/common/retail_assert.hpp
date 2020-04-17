/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "gaia_exception.hpp"

using namespace std;

namespace gaia
{
namespace common
{

// Retail asserts are meant for important conditions that indicate internal errors.
// They help us surface issues early, at the source.
inline void retail_assert(bool condition, string message)
{
    if (!condition)
    {
        throw gaia_exception(message);
    }
}

}
}
