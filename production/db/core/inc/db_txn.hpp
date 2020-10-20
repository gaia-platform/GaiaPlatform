/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{

// Returns the current transaction id.
gaia::db::gaia_txn_id_t get_txn_id();

// Returns the begin_ts of the transaction
gaia::db::gaia_txn_id_t get_begin_ts();

} // namespace db
} // namespace gaia
