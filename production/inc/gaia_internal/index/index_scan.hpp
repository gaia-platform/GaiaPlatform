/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"
#include "index_scan_iterator.hpp"
#include "se_helpers.hpp"

// Read API for index for the client.
// This object is a proxy for the actual index.
// Scan objects require an active txn

namespace gaia
{
namespace db
{
namespace index
{

class index_scan_t
{
private:
    se_index_t idx;
    //std::unique_ptr<base_index_scan_iterator_t> scan_iterator;

public:
    explicit index_scan_t(gaia::common::gaia_id_t index_id)
        : idx(get_indexes(index_id))
    {
    }
};

} // namespace index
} // namespace db
} // namespace gaia
