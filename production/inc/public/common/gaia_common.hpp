/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <limits>
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
 * The value of an invalid gaia_type.
 */
constexpr gaia_type_t INVALID_GAIA_TYPE = 0;

/**
 * The type of a Gaia event type.
 */
typedef uint8_t gaia_event_t;

/**
 * The position of a field in a Gaia table.
 */
typedef uint16_t field_position_t;

/**
 * List of field positions.
 */
typedef std::vector<field_position_t> field_position_list_t;

/**
 * Locates the pointer to a reference within the references array
 * in the Gaia Object.
 *
 * @see gaia::common::db::relationship_t
 */
 // TODO this should be changed to unit8_t to match the Catalog (or vice-versa)
typedef uint16_t reference_offset_t;

/**
 * The value of an invalid reference_offset_t.
 */
constexpr reference_offset_t INVALID_REFERENCE_OFFSET = std::numeric_limits<reference_offset_t>::max(); // NOLINT

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
