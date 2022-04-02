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
 * @brief Checks whether a DDL session is open.
 */
bool is_ddl_session_open();

/**
 * @brief Special version of begin_session() used for DDL operations.
 */
void begin_ddl_session();

/**
 * @brief Sets the DB client's commit trigger function.
 */
void set_commit_trigger(gaia::db::triggers::commit_trigger_fn trigger_fn);

/**
 * @brief Reinitializes the DB client's shared memory structures.
 * For use only by test code, in combination with the DB
 * server's reinitialization feature.
 */
void clear_shared_memory();

/**
 * @brief Internal API for getting the begin_ts of the current txn.
 */
gaia_txn_id_t get_current_txn_id();

// The name of the SE server binary.
constexpr char c_db_server_exec_name[] = "gaia_db_server";

// The name of the default gaia instance.
constexpr char c_default_instance_name[] = "gaia_default_instance";

// The customer facing name of the DB server.
constexpr char c_db_server_name[] = "Gaia Database Server";

} // namespace db
} // namespace gaia
