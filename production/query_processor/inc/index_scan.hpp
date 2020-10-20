/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"

#include "gaia_internal/db/gaia_ptr.hpp"

#include "db_shared_data.hpp"
#include "index_scan_impl.hpp"
#include "qp_operator.hpp"

// Read API for index for the client.
// This object is a proxy for the actual index.
// Scan objects require an active txn

namespace gaia
{
namespace qp
{
namespace scan
{

class index_scan_t : physical_operator_t
{
private:
    std::unique_ptr<base_index_scan_iterator_t> scan_impl;

public:
    explicit index_scan_t(common::gaia_id_t index_id);

    index_scan_t operator++();
    index_scan_t operator++(int);

    db::gaia_ptr_t& operator*() const;
    db::gaia_ptr_t& operator*();
    db::gaia_ptr_t* operator->() const;
    db::gaia_ptr_t* operator->();

    bool operator==(const index_scan_t&) const;
    bool operator!=(const index_scan_t&) const;
    bool operator!=(std::nullptr_t) const;
};

} // namespace scan
} // namespace qp
} // namespace gaia
