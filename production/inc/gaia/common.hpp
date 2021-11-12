/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <limits>
#include <vector>

#include "gaia/int_type.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

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

constexpr char c_empty_string[] = "";

/**
 * The type of a Gaia object identifier.
 */
class gaia_id_t : public int_type_t<uint64_t>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr gaia_id_t()
        : int_type_t<uint64_t>()
    {
    }

    constexpr gaia_id_t(uint64_t value)
        : int_type_t<uint64_t>(value)
    {
    }
};

static_assert(
    sizeof(gaia_id_t) == sizeof(gaia_id_t::value_type),
    "gaia_id_t has a different size than its underlying integer type!");

/**
 * The value of an invalid gaia_id_t.
 */
constexpr gaia_id_t c_invalid_gaia_id;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(c_invalid_gaia_id.value() == 0, "Invalid c_invalid_gaia_id initialization!");

/**
 * The type of a Gaia type identifier.
 */
typedef uint32_t gaia_type_t;

/**
 * The value of an invalid gaia_type_t.
 */
constexpr gaia_type_t c_invalid_gaia_type = 0;

/**
 * Opaque handle to a gaia record;
 */
typedef uint64_t gaia_handle_t;

/**
 * The type of a Gaia event type.
 */
typedef uint8_t gaia_event_t;

/**
 * The position of a field in a Gaia table.
 */
typedef uint16_t field_position_t;

/**
 * The value of an invalid field_position_t.
 */
constexpr field_position_t c_invalid_field_position = std::numeric_limits<field_position_t>::max();

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
typedef uint16_t reference_offset_t;

/**
 * The value of an invalid field_position_t.
 */
constexpr reference_offset_t c_invalid_reference_offset = std::numeric_limits<reference_offset_t>::max();

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
};

template <typename T>
constexpr std::underlying_type_t<T> get_enum_value(T val)
{
    return static_cast<std::underlying_type_t<T>>(val);
}

constexpr char c_whitespace_chars[] = " \n\r\t\f\v";

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

namespace std
{

// This enables gaia_id_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::common::gaia_id_t>
{
    size_t operator()(const gaia::common::gaia_id_t& gaia_id) const noexcept
    {
        return std::hash<gaia::common::gaia_id_t::value_type>()(gaia_id.value());
    }
};

} // namespace std

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
