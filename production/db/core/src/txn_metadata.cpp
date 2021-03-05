/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "txn_metadata.hpp"

#include "gaia_internal/common/retail_assert.hpp"

namespace gaia
{
namespace db
{

static_assert(
    sizeof(txn_metadata_t) == sizeof(uint64_t),
    "txn_metadata_t struct should only contain a uint64_t value!");

txn_metadata_t::txn_metadata_t() noexcept
    : value(c_value_uninitialized)
{
}

txn_metadata_t::txn_metadata_t(uint64_t value)
    : value(value)
{
}

const char* txn_metadata_t::status_to_str() const
{
    common::retail_assert(
        !is_uninitialized() && !is_sealed(),
        "Not a valid txn metadata!");

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
        common::retail_assert(false, "Unexpected txn_metadata_t status flags!");
    }

    return nullptr;
}

} // namespace db
} // namespace gaia
