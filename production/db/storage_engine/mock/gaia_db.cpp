/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_db.hpp"
#include "storage_engine_client.hpp"

bool gaia::db::is_transaction_active() {
    return gaia::db::client::is_transaction_active();
}

void gaia::db::begin_session() {
    gaia::db::client::begin_session();
}

void gaia::db::end_session() {
    gaia::db::client::end_session();
}

void gaia::db::begin_transaction() {
    gaia::db::client::begin_transaction();
}

void gaia::db::rollback_transaction() {
    gaia::db::client::rollback_transaction();
}

bool gaia::db::commit_transaction() {
    return gaia::db::client::commit_transaction();
}
