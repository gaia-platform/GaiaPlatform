/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"

namespace gaia
{
namespace qp
{

// Base physical operator class
// This is mostly a proxy for db client methods
class physical_operator_t
{
protected:
    static void verify_txn_active();

    static void init_index_stream(common::gaia_id_t index_id);

    //TODO: scaffolding for index_scan, method should go away once we have live shmem indexes
};

} // namespace qp
} // namespace gaia
