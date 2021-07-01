/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <atomic>
#include <ostream>

#include "gaia/common.hpp"

#include "gaia_internal/common/mmap_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{

enum class txn_decision_type_t : uint8_t
{
    not_set = 0,
    commit = 1,
    abort = 2,
};

struct decision_record_entry_t
{
    gaia_txn_id_t txn_commit_ts;
    txn_decision_type_t decision;
};

typedef std::vector<std::pair<gaia_txn_id_t, txn_decision_type_t>> decision_list_t;

} // namespace db
} // namespace gaia
