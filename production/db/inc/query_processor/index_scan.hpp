////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"

#include "gaia_internal/db/gaia_ptr.hpp"

#include "db_shared_data.hpp"
#include "predicate.hpp"

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
 * Additional scan state for index scans.
 *
 * In future could be modified to also collect statistics.
 */
class scan_state_t
{
public:
    scan_state_t() = default;
    explicit scan_state_t(std::shared_ptr<index_predicate_t> predicate);
    scan_state_t(std::shared_ptr<index_predicate_t> predicate, size_t limit);

    // Returns true for rows fulfilling predicate conditions or end marker.
    bool should_return_row(gaia_ptr_t ptr);

    // Check if we have hit the limit for this scan.
    inline bool limit_reached();

    std::shared_ptr<index_predicate_t> predicate();

private:
    std::shared_ptr<index_predicate_t> m_predicate;
    bool m_has_limit;
    size_t m_limit_rows_remaining;
};

inline bool scan_state_t::limit_reached()
{
    return m_has_limit && m_limit_rows_remaining == 0;
}

/**
 * Iterator interface over the index scan object.
 *
 * Returns gaia_ptr_t from the index implementation.
 *
 * Instantiating the iterator will cause the init() method
 * of the implementation to be called.
 *
 */
class index_scan_iterator_t
{
public:
    index_scan_iterator_t(common::gaia_id_t index_id, scan_state_t scan_state);

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
    scan_state_t m_scan_state;
};

/**
 * This method is a point of entry and is compatible with C++'s range-based for loop.
 *
 * Example usage:
 *
 *   index_scan_t scan (index_id);
 *   for (const auto& it: scan)
 *   {
 *     ...
 *   }
 */
class index_scan_t
{
public:
    explicit index_scan_t(common::gaia_id_t index_id, std::shared_ptr<index_predicate_t> predicate = nullptr)
        : m_index_id(index_id), m_predicate(predicate), m_has_limit(false), m_limit(0)
    {
    }

    index_scan_t(common::gaia_id_t index_id, std::shared_ptr<index_predicate_t> predicate, size_t limit)
        : m_index_id(index_id), m_predicate(predicate), m_has_limit(true), m_limit(limit)
    {
    }

    index_scan_iterator_t begin();
    std::nullptr_t end();

private:
    common::gaia_id_t m_index_id;
    std::shared_ptr<index_predicate_t> m_predicate;
    bool m_has_limit;
    size_t m_limit;
};

} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
