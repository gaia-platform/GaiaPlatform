/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/db/db.hpp"
#include "db_types.hpp"
#include "gaia_db_internal.hpp"
#include "se_client.hpp"

bool gaia::db::is_transaction_active()
{
    return gaia::db::client::is_transaction_active();
}

void gaia::db::begin_session()
{
    gaia::db::client::begin_session();
}

void gaia::db::end_session()
{
    gaia::db::client::end_session();
}

void gaia::db::begin_transaction()
{
    gaia::db::client::begin_transaction();
}

void gaia::db::rollback_transaction()
{
    gaia::db::client::rollback_transaction();
}

void gaia::db::commit_transaction()
{
    gaia::db::client::commit_transaction();
}

void gaia::db::set_commit_trigger(gaia::db::triggers::commit_trigger_fn trigger_fn)
{
    gaia::db::client::set_commit_trigger(trigger_fn);
}

void gaia::db::clear_shared_memory()
{
    gaia::db::client::clear_shared_memory();
}
