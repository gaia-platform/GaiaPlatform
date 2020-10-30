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
constexpr gaia_id_t c_invalid_gaia_id = 0;

/**
 * The type of a Gaia type identifier.
 */
typedef uint32_t gaia_type_t;

/**
 * The value of an invalid gaia_type_t.
 */
constexpr gaia_type_t c_invalid_gaia_type = 0;

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

/*
 * Data types for Gaia field records.
 */
enum class data_type_t : uint8_t
{
    e_bool,
    e_int8,
    e_uint8,
    e_int16,
    e_uint16,
    e_int32,
    e_uint32,
    e_int64,
    e_uint64,
    e_float,
    e_double,
    e_string,
    e_references
};

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
