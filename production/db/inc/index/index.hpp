/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include <data_holder.hpp>

namespace gaia
{
namespace db
{
namespace index
{

struct index_record_t
{
    std::vector<gaia::db::types::data_holder_t> key_values;
    uint64_t row_id;
    uint64_t transaction_id;
    uint8_t deleted;

    index_record_t() = default;
    ~index_record_t() = default;

    int compare(const index_record_t& other) const;
};

}
}
}
