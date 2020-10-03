/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include <db_types.hpp>
#include <data_holder.hpp>

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
    gaia_xid_t transaction_id;
    uint8_t deleted;

    index_record_t() = default;
    ~index_record_t() = default;

    int compare(const index_record_t& other) const;
};

}
}
}
