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

// This object is the physical scan query node for indexes.

namespace gaia
{
namespace qp
{
namespace scan
{

/**
 * Physical index scan operator.
 **/
class index_scan_t
{
private:
    common::gaia_id_t m_index_id;

public:
    explicit index_scan_t(common::gaia_id_t index_id)
        : m_index_id(index_id)
    {
    }
    index_scan_iterator_t begin();
    std::nullptr_t end();
};

/**
 * Iterator interface over the index scan object
 **/
class index_scan_iterator_t
{
private:
    common::gaia_id_t m_index_id;
    std::shared_ptr<base_index_scan_impl_t> m_scan_impl;
    db::gaia_ptr_t m_gaia_ptr;

    index_scan_iterator_t() = default;

public:
    explicit index_scan_iterator_t(common::gaia_id_t index_id);

    index_scan_iterator_t operator++();
    index_scan_iterator_t operator++(int);

    db::gaia_ptr_t operator*() const;
    db::gaia_ptr_t* operator->();

    bool operator==(const index_scan_iterator_t&) const;
    bool operator==(std::nullptr_t) const;
    bool operator!=(const index_scan_iterator_t&) const;
    bool operator!=(std::nullptr_t) const;
};

} // namespace scan
} // namespace qp
} // namespace gaia
