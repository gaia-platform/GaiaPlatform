/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia_internal/db/db_types.hpp"

#include "index.hpp"

namespace gaia
{
namespace db
{
namespace index
{

/**
 * Class iterating over indexes, with locks handled via RAII.
 */

template <typename T_structure, typename T_index_it>
class locking_iterator_t
{
public:
    locking_iterator_t() = default;
    locking_iterator_t(base_index_t* db_idx, T_index_it iter, T_index_it end_iter);
    locking_iterator_t(const locking_iterator_t& other);
    ~locking_iterator_t();

    locking_iterator_t& operator++();

    locking_iterator_t operator++(int);

    locking_iterator_t& next_key() const;
    const typename T_index_it::value_type& operator*() const;

    const typename T_index_it::value_type* operator->() const;

    bool operator==(const locking_iterator_t& other) const;

    bool operator!=(const locking_iterator_t& other) const;

private:
    base_index_t* m_db_idx{nullptr};
    T_index_it m_index_it;
    T_index_it m_end_it;
};

#include "index_iterator.inc"

} // namespace index
} // namespace db
} // namespace gaia
