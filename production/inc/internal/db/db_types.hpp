/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace gaia {
namespace db {

/**
 * The type of a Gaia transaction id.
 *
 * This type is used for both transaction begin and commit timestamps, which are
 * obtained by incrementing a global atomic counter in shared memory.
 */
typedef uint64_t gaia_txn_id_t;

/**
 * The type of a Gaia locator id.
 *
 * A locator is an array index in a global shared memory segment, whose entry
 * holds the offset of the corresponding gaia_se_object in the shared memory
 * data segment. Each gaia_id corresponds to a unique locator for the lifetime
 * of the server process.
 */
typedef uint64_t gaia_locator_t;

/**
 * The type of a Gaia data offset.
 *
 * This represents the offset of a gaia_se_object in the global data shared
 * memory segment, which can be added to the local base address of the mapped
 * data segment to obtain a valid local pointer to the object.
 */
typedef uint64_t gaia_offset_t;

}  // namespace db
}  // namespace gaia
