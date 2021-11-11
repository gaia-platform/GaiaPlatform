/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <limits>

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
typedef uint64_t gaia_txn_id_t;

/**
 * The value of an invalid gaia_txn_id.
 */
constexpr gaia_txn_id_t c_invalid_gaia_txn_id = 0;

/**
 * Number of reserved system gaia_txn_ids.
 */
constexpr size_t c_num_reserved_gaia_txn_ids = 4;

/**
 * The value of a frozen gaia_txn_id.
 */
constexpr gaia_txn_id_t c_frozen_gaia_txn_id = std::numeric_limits<uint64_t>::max();

/**
 * The value of the last valid user gaia_txn_id.
 */
constexpr gaia_txn_id_t c_last_user_txn_id = std::numeric_limits<uint64_t>::max() - c_num_reserved_gaia_txn_ids + 1;

/**
 * The type of a Gaia locator id.
 *
 * A locator is an array index in a global shared memory segment, whose entry
 * holds the offset of the corresponding db_object_t in the shared memory
 * data segment. Each gaia_id corresponds to a unique locator for the lifetime
 * of the server process.
 */
typedef uint64_t gaia_locator_t;

/**
 * The value of an invalid gaia_locator.
 */
constexpr gaia_locator_t c_invalid_gaia_locator = 0;

/**
 * The value of the first gaia_locator.
 */
constexpr gaia_locator_t c_first_gaia_locator = c_invalid_gaia_locator + 1;

/**
 * The type of a Gaia data offset.
 *
 * This represents the 64-byte-aligned offset of a db_object_t in the global
 * data shared memory segment, which can be added to the local base address of
 * the mapped data segment to obtain a valid local pointer to the object.
 */
typedef uint32_t gaia_offset_t;

/**
 * The value of an invalid gaia_offset.
 */
constexpr gaia_offset_t c_invalid_gaia_offset = 0;

} // namespace db
} // namespace gaia
