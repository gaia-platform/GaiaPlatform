/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

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

/**
 * Reinitializes the DB client's shared memory structures.
 * For use only by test code, in combination with the DB
 * server's reinitialization feature.
 */
void clear_shared_memory();

// Todo (Mihir): Expose options to set the persistent
// directory path & also some way to destroy it instead
// of hardcoding the path.
// https://gaiaplatform.atlassian.net/browse/GAIAPLAT-310
constexpr char PERSISTENT_DIRECTORY_PATH[] = "/tmp/gaia_db";

}  // namespace db
}  // namespace gaia
