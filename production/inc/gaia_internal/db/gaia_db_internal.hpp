/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/db/triggers.hpp"

namespace gaia
{
namespace db
{

/**
 * Sets the DB client's commit trigger function.
 */
void set_commit_trigger(gaia::db::triggers::commit_trigger_fn trigger_fn);

/**
 * Reinitializes the DB client's shared memory structures.
 * For use only by test code, in combination with the DB
 * server's reinitialization feature.
 */
void clear_shared_memory();

// The name of the SE server binary.
constexpr char c_db_server_exec_name[] = "gaia_db_server";

// Used by the SE server and client to bind and connect to the server's listening socket.
constexpr char c_db_server_socket_name[] = "gaia_db_server_socket";

} // namespace db
} // namespace gaia
