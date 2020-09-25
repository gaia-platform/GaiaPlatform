/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia {
namespace db {

/**
 * The type of a Gaia transaction id.
 */
typedef uint64_t gaia_xid_t;

/**
 * The type of a Gaia locator id.
 */
typedef uint64_t gaia_locator_t;

/**
 * The type of a Gaia data offset.
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
