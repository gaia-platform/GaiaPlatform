/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup Common
 * @{
 */

/**
 * The type of a Gaia object identifier.
 */
typedef uint64_t gaia_id_t;

/**
 * The type of a Gaia type identifier.
 */
typedef uint64_t gaia_type_t;

/**
 * The type of a Gaia transaction hook.
 */
typedef void (* gaia_tx_hook)(void);

} // common
/*@}*/
} // gaia
