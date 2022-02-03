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
 * \addtogroup gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup common
 * @{
 */

constexpr char c_empty_string[] = "";

/**
 * The type of a Gaia object identifier.
 */
class gaia_id_t : public int_type_t<uint64_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr gaia_id_t()
        : int_type_t<uint64_t, 0>()
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr gaia_id_t(uint64_t value)
        : int_type_t<uint64_t, 0>(value)
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
static_assert(
    c_invalid_gaia_id.value() == gaia_id_t::c_default_invalid_value,
    "Invalid c_invalid_gaia_id initialization!");

/**
 * The type of a Gaia type identifier.
 */
class gaia_type_t : public int_type_t<uint32_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr gaia_type_t()
        : int_type_t<uint32_t, 0>()
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr gaia_type_t(uint32_t value)
        : int_type_t<uint32_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(gaia_type_t) == sizeof(gaia_type_t::value_type),
    "gaia_type_t has a different size than its underlying integer type!");

/**
 * The value of an invalid gaia_type_t.
 */
constexpr gaia_type_t c_invalid_gaia_type;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_gaia_type.value() == gaia_type_t::c_default_invalid_value,
    "Invalid c_invalid_gaia_type initialization!");

/**
 * Opaque handle to a gaia record.
 */
class gaia_handle_t : public int_type_t<uint64_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr gaia_handle_t()
        : int_type_t<uint64_t, 0>()
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr gaia_handle_t(uint64_t value)
        : int_type_t<uint64_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(gaia_handle_t) == sizeof(gaia_handle_t::value_type),
    "gaia_handle_t has a different size than its underlying integer type!");

/**
 * The type of a Gaia event type.
 */
class gaia_event_t : public int_type_t<uint8_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr gaia_event_t()
        : int_type_t<uint8_t, 0>()
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr gaia_event_t(uint8_t value)
        : int_type_t<uint8_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(gaia_event_t) == sizeof(gaia_event_t::value_type),
    "gaia_event_t has a different size than its underlying integer type!");

/**
 * The position of a field in a Gaia table.
 */
class field_position_t : public int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr field_position_t()
        : int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>()
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr field_position_t(uint16_t value)
        : int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>(value)
    {
    }
};

static_assert(
    sizeof(field_position_t) == sizeof(field_position_t::value_type),
    "field_position_t has a different size than its underlying integer type!");

/**
 * The value of an invalid field_position_t.
 */
constexpr field_position_t c_invalid_field_position;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_field_position.value() == field_position_t::c_default_invalid_value,
    "Invalid c_invalid_field_position initialization!");

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
class reference_offset_t : public int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr reference_offset_t()
        : int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>()
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr reference_offset_t(uint16_t value)
        : int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>(value)
    {
    }
};

static_assert(
    sizeof(reference_offset_t) == sizeof(reference_offset_t::value_type),
    "reference_offset_t has a different size than its underlying integer type!");

/**
 * The value of an invalid field_position_t.
 */
constexpr reference_offset_t c_invalid_reference_offset;

// The offset of the parent reference in an anchor node.
constexpr common::reference_offset_t c_ref_anchor_parent_offset{0};
// The offset of the first child reference in an anchor node.
constexpr common::reference_offset_t c_ref_anchor_first_child_offset{1};
// Total number of reference slots in an anchor node.
constexpr common::reference_offset_t c_ref_anchor_ref_num{2};

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_reference_offset.value() == reference_offset_t::c_default_invalid_value,
    "Invalid c_invalid_reference_offset initialization!");

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
constexpr size_t c_uint64_bit_count = std::numeric_limits<uint64_t>::digits;

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

// This enables gaia_type_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::common::gaia_type_t>
{
    size_t operator()(const gaia::common::gaia_type_t& gaia_type) const noexcept
    {
        return std::hash<gaia::common::gaia_type_t::value_type>()(gaia_type.value());
    }
};

// This enables gaia_handle_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::common::gaia_handle_t>
{
    size_t operator()(const gaia::common::gaia_handle_t& gaia_handle) const noexcept
    {
        return std::hash<gaia::common::gaia_handle_t::value_type>()(gaia_handle.value());
    }
};

// This enables gaia_event_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::common::gaia_event_t>
{
    size_t operator()(const gaia::common::gaia_event_t& gaia_event) const noexcept
    {
        return std::hash<gaia::common::gaia_event_t::value_type>()(gaia_event.value());
    }
};

// This enables field_position_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::common::field_position_t>
{
    size_t operator()(const gaia::common::field_position_t& field_position) const noexcept
    {
        return std::hash<gaia::common::field_position_t::value_type>()(field_position.value());
    }
};

// This enables reference_offset_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::common::reference_offset_t>
{
    size_t operator()(const gaia::common::reference_offset_t& reference_offset) const noexcept
    {
        return std::hash<gaia::common::reference_offset_t::value_type>()(reference_offset.value());
    }
};

} // namespace std

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
