/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"

#include "gaia_internal/db/gaia_ptr.hpp"

#include "db_shared_data.hpp"

namespace gaia
{
namespace db
{
namespace query_processor
{
namespace scan
{

class base_index_scan_physical_t;

/**
 * Iterator interface over the index scan object.
 *
 * Returns gaia_ptr_t from the index implementation.
 *
 * Instantiating the iterator will cause the init() method
 * of the implementation to be called.
 *
 **/
class index_scan_iterator_t
{
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

private:
    common::gaia_id_t m_index_id;
    std::shared_ptr<base_index_scan_physical_t> m_scan_impl;
    gaia_ptr_t m_gaia_ptr;

    index_scan_iterator_t() = default;
};

/**
 *  This method is a point of entry and is compatible with C++'s range-based for loop.
 *
 * Example usage:
 *
 *   index_scan_t scan (index_id);
 *   for (const auto& it: scan)
 *   {
 *     ...
 *   }
 *
 **/
class index_scan_t
{
public:
    explicit index_scan_t(common::gaia_id_t index_id)
        : m_index_id(index_id)
    {
    }
    index_scan_iterator_t begin();
    std::nullptr_t end();

private:
    common::gaia_id_t m_index_id;
};

} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
