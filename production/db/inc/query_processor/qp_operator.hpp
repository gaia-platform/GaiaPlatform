/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"

#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{
namespace query_processor
{

/**
 * Base physical operator class.
 * This is mostly a proxy for db client methods right now.
 *
 * In future, this class should be much more well fleshed out,
 * perhaps with methods like explain()?
 */
class physical_operator_t
{
};

class db_client_proxy_t
{
public:
    static void verify_txn_active();
    static gaia::db::gaia_txn_id_t get_current_txn_id();

    // The client maintains local indexes for uncommitted changes.
    // These are used to augment the server index information.
    // The merging is performed in the physical index scan implementation.
    static void update_local_indexes();
};

} // namespace query_processor
} // namespace db
} // namespace gaia
