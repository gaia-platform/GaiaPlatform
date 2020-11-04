/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "triggers.hpp"

namespace gaia {
namespace db {

/**
 * Sets the DB client's commit trigger function.
 * Can only be called once.
 * Returns true if the trigger was successfully initialized,
 * false if it was already set.
 */
bool set_commit_trigger(gaia::db::triggers::commit_trigger_fn trigger_fn);

/**
 * Reinitializes the DB client's shared memory structures.
 * For use only by test code, in combination with the DB
 * server's reinitialization feature.
 */
void clear_shared_memory();

gaia_type_t allocate_type_id();

// Todo (Mihir): Expose options to set the persistent
// directory path & also some way to destroy it instead
// of hardcoding the path.
// https://gaiaplatform.atlassian.net/browse/GAIAPLAT-310
constexpr char PERSISTENT_DIRECTORY_PATH[] = "/tmp/gaia_db";

// The name of the SE server binary.
constexpr char SE_SERVER_EXEC_NAME[] = "gaia_se_server";

// Used by the SE server and client to bind and connect to the server's listening socket.
constexpr char SE_SERVER_SOCKET_NAME[] = "gaia_se_server_socket";

}  // namespace db
}  // namespace gaia
