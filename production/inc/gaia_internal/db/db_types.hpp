/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <functional>

#include "gaia/int_type.hpp"

namespace gaia
{
namespace db
{

/**
 * The type of a Gaia transaction id.
 *
 * This type is used for both transaction begin and commit timestamps, which are
 * obtained by incrementing a global atomic counter in shared memory.
 */
class gaia_txn_id_t : public common::int_type_t<uint64_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr gaia_txn_id_t()
        : common::int_type_t<uint64_t, 0>()
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr gaia_txn_id_t(uint64_t value)
        : common::int_type_t<uint64_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(gaia_txn_id_t) == sizeof(gaia_txn_id_t::value_type),
    "gaia_txn_id_t has a different size than its underlying integer type!");

/**
 * The value of an invalid gaia_txn_id.
 */
constexpr gaia_txn_id_t c_invalid_gaia_txn_id;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_gaia_txn_id.value() == gaia_txn_id_t::c_default_invalid_value,
    "Invalid c_invalid_gaia_txn_id initialization!");

/**
 * The type of a Gaia locator id.
 *
 * A locator is an array index in a global shared memory segment, whose entry
 * holds the offset of the corresponding db_object_t in the shared memory
 * data segment. Each gaia_id corresponds to a unique locator for the lifetime
 * of the server process.
 */
class gaia_locator_t : public common::int_type_t<uint64_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr gaia_locator_t()
        : common::int_type_t<uint64_t, 0>()
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr gaia_locator_t(uint64_t value)
        : common::int_type_t<uint64_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(gaia_locator_t) == sizeof(gaia_locator_t::value_type),
    "gaia_locator_t has a different size than its underlying integer type!");

/**
 * The value of an invalid gaia_locator.
 */
constexpr gaia_locator_t c_invalid_gaia_locator;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_gaia_locator.value() == gaia_locator_t::c_default_invalid_value,
    "Invalid c_invalid_gaia_locator initialization!");

/**
 * The value of the first gaia_locator.
 */
constexpr gaia_locator_t c_first_gaia_locator = c_invalid_gaia_locator.value() + 1;

/**
 * The type of a Gaia data offset.
 *
 * This represents the 64-byte-aligned offset of a db_object_t in the global
 * data shared memory segment, which can be added to the local base address of
 * the mapped data segment to obtain a valid local pointer to the object.
 */
class gaia_offset_t : public common::int_type_t<uint32_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr gaia_offset_t()
        : common::int_type_t<uint32_t, 0>()
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr gaia_offset_t(uint32_t value)
        : common::int_type_t<uint32_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(gaia_offset_t) == sizeof(gaia_offset_t::value_type),
    "gaia_offset_t has a different size than its underlying integer type!");

/**
 * The value of an invalid gaia_offset.
 */
constexpr gaia_offset_t c_invalid_gaia_offset;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_gaia_offset.value() == gaia_offset_t::c_default_invalid_value,
    "Invalid c_invalid_gaia_offset initialization!");

} // namespace db
} // namespace gaia

namespace std
{

// This enables gaia_txn_id_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::db::gaia_txn_id_t>
{
    size_t operator()(const gaia::db::gaia_txn_id_t& txn_id) const noexcept
    {
        return std::hash<gaia::db::gaia_txn_id_t::value_type>()(txn_id.value());
    }
};

// This enables gaia_locator_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::db::gaia_locator_t>
{
    size_t operator()(const gaia::db::gaia_locator_t& locator) const noexcept
    {
        return std::hash<gaia::db::gaia_locator_t::value_type>()(locator.value());
    }
};

// This enables gaia_offset_t to be hashed and used as a key in maps.
template <>
struct hash<gaia::db::gaia_offset_t>
{
    size_t operator()(const gaia::db::gaia_offset_t& offset) const noexcept
    {
        return std::hash<gaia::db::gaia_offset_t::value_type>()(offset.value());
    }
};

} // namespace std
