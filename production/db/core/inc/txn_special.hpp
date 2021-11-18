
/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia
{
namespace db
{
namespace transactions
{

// Special ts values/masks.
constexpr uint64_t c_txn_ts_frozen_shift{63ULL};
constexpr uint64_t c_txn_ts_frozen_mask{1ULL << c_txn_ts_frozen_shift};

constexpr bool is_frozen_ts(gaia_txn_id_t ts)
{
    return (ts & c_txn_ts_frozen_mask) == c_txn_ts_frozen_mask;
}

constexpr gaia_txn_id_t set_ts_frozen(gaia_txn_id_t ts)
{
    return ts | c_txn_ts_frozen_mask;
}

constexpr gaia_txn_id_t clear_ts_frozen(gaia_txn_id_t ts)
{
    return ts & ~c_txn_ts_frozen_mask;
}

} // namespace transactions
} // namespace db
} // namespace gaia
