/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/db/db.hpp"

#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

#include "db_client.hpp"

bool gaia::db::is_transaction_active()
{
    return gaia::db::client_t::is_transaction_active();
}

void gaia::db::begin_session(gaia::db::session_opts_t session_opts)
{
    gaia::db::client_t::begin_session(session_opts);
}

void gaia::db::end_session()
{
    gaia::db::client_t::end_session();
}

void gaia::db::begin_transaction()
{
    gaia::db::client_t::begin_transaction();
}

void gaia::db::rollback_transaction()
{
    gaia::db::client_t::rollback_transaction();
}

void gaia::db::commit_transaction()
{
    gaia::db::client_t::commit_transaction();
}

void gaia::db::set_commit_trigger(gaia::db::triggers::commit_trigger_fn trigger_fn)
{
    gaia::db::client_t::set_commit_trigger(trigger_fn);
}

void gaia::db::clear_shared_memory()
{
    gaia::db::client_t::clear_shared_memory();
}

gaia::db::gaia_txn_id_t gaia::db::get_txn_id()
{
    return gaia::db::client_t::get_txn_id();
}
