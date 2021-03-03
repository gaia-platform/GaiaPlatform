/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "ts_info.hpp"

#include "gaia_internal/common/retail_assert.hpp"

namespace gaia
{
namespace db
{

ts_info_t::ts_info_t(uint64_t value)
{
    this->value = value;
}

const char* ts_info_t::status_to_str()
{
    common::retail_assert(
        !is_unknown() && !is_invalid(),
        "Not a valid timestamp info!");

    uint64_t status = get_status();
    switch (status)
    {
    case c_txn_status_active:
        return "ACTIVE";
    case c_txn_status_submitted:
        return "SUBMITTED";
    case c_txn_status_terminated:
        return "TERMINATED";
    case c_txn_status_validating:
        return "VALIDATING";
    case c_txn_status_committed:
        return "COMMITTED";
    case c_txn_status_aborted:
        return "ABORTED";
    default:
        common::retail_assert(false, "Unexpected ts_info_t status flags!");
    }

    return nullptr;
}

} // namespace db
} // namespace gaia
