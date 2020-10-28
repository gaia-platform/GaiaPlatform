/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <limits>
#include <vector>

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
 * The value of an invalid gaia_id_t.
 */
constexpr gaia_id_t INVALID_GAIA_ID = 0;

/**
 * The type of a Gaia type identifier.
 */
typedef uint32_t gaia_type_t;

/**
 * The value of an invalid gaia_type_t.
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
// TODO this should be changed to uint8_t to match the Catalog (or vice-versa)
typedef uint16_t reference_offset_t;

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
