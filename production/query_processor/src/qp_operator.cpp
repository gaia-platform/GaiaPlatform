/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "qp_operator.hpp"

#include "db_client.hpp"

void gaia::qp::physical_operator_t::verify_txn_active()
{
    db::client_t::verify_txn_active();
}
