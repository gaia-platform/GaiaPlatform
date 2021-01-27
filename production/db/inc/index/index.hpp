/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <vector>

#include "data_holder.hpp"

#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{
namespace index
{

struct index_record_t
{
    std::vector<gaia::db::payload_types::data_holder_t> key_values;
    gaia_locator_t locator;
    gaia_txn_id_t txn_id;
    uint8_t deleted;

    index_record_t() = default;
    ~index_record_t() = default;

    int compare(const index_record_t& other) const;
};

} // namespace index
} // namespace db
} // namespace gaia
