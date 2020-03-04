/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <stdint.h>

namespace gaia 
{
/**
 * \addtogroup Gaia
 * @{
 * Public application programming interface to the Gaia sub-system.
 * 
 * The API set includes functionality for extending user data classes,
 * logging events, and executing rules.  Data accessed via the extended
 * data classes operate over the Gaia in-memory transactional store.
 */    
namespace common
{

typedef uint64_t gaia_type_t; // temporary place for gaia type indicator

/**
 * All objects that are managed by the gaia platform must inherit from gaia_base
 */
class gaia_base 
{
public:
    virtual ~gaia_base() = default;
};

}

/*@}*/
}
