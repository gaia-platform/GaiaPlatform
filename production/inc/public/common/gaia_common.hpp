/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <vector>

namespace gaia {
/**
 * \addtogroup Gaia
 * @{
 */
namespace common {
/**
 * \addtogroup Common
 * @{
 */

/**
 * The type of a Gaia object identifier.
 */
typedef uint64_t gaia_id_t;

/**
 * The value of an invalid gaia_id.
 */
constexpr gaia_id_t INVALID_GAIA_ID = 0;

/**
 * The type of a Gaia type identifier.
 */
typedef uint64_t gaia_type_t;

/**
 * The type of a Gaia transaction id.
 */
typedef uint64_t gaia_xid_t;

/**
 * The type of a Gaia event type.
 */
typedef uint8_t gaia_event_t;

}  // namespace common
/*@}*/
}  // namespace gaia
