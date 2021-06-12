/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <atomic>

namespace gaia
{
namespace db
{

// Convenience method to safely cast from a scoped enum to its underlying type.
template <typename T>
constexpr auto to_integral(T e)
{
    return static_cast<std::underlying_type_t<T>>(e);
}

// This overload is for scoped enums nested in atomic types.
template <typename T>
constexpr auto to_integral(const std::atomic<T>& e)
{
    return static_cast<std::underlying_type_t<T>>(e.load());
}

template <typename T>
constexpr auto from_integral(size_t n)
{
    return T{static_cast<std::underlying_type_t<T>>(n)};
}

/**
 * The type of a Gaia transaction id.
 *
 * This type is used for both transaction begin and commit timestamps, which are
 * obtained by incrementing a global atomic counter in shared memory.
 */
enum class gaia_txn_id_t : uint64_t
{
};

/**
 * The value of an invalid gaia_txn_id.
 */
constexpr gaia_txn_id_t c_invalid_gaia_txn_id{0};

/**
 * The type of a Gaia locator id.
 *
 * A locator is an array index in a global shared memory segment, whose entry
 * holds the offset of the corresponding db_object_t in the shared memory
 * data segment. Each gaia_id corresponds to a unique locator for the lifetime
 * of the server process.
 */
enum class gaia_locator_t : uint64_t
{
};

/**
 * The value of an invalid gaia_locator.
 */
constexpr gaia_locator_t c_invalid_gaia_locator{0};

/**
 * The value of the first gaia_locator.
 */
constexpr gaia_locator_t c_first_gaia_locator{
    to_integral(c_invalid_gaia_locator) + 1};

/**
 * The type of a Gaia data offset.
 *
 * This represents the 64-byte-aligned offset of a db_object_t in the global
 * data shared memory segment, which can be added to the local base address of
 * the mapped data segment to obtain a valid local pointer to the object.
 */
enum class gaia_offset_t : uint32_t
{
};

/**
 * The value of an invalid gaia_offset.
 */
constexpr gaia_offset_t c_invalid_gaia_offset{0};

} // namespace db
} // namespace gaia
