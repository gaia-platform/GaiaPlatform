/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <stdint.h>

namespace gaia 
{

namespace api 
{

typedef uint64_t gaia_type_t; // temporary place for gaia type indicator

/**
 * All objects that are managed by the gaia platform must inherit from gaia_base
 * 
 */
class gaia_base 
{
    virtual ~gaia_base() = default;
};

}
}