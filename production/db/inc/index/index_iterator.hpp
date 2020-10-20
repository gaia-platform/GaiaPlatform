/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "index.hpp"
#include "db_types.hpp"

namespace gaia
{
namespace db
{
namespace index
{

template <typename T_index, typename T_index_iter>
class locking_iterator_t
{
private:
    base_index_t* se_idx;
    T_index_iter index_iter;

public:
    locking_iterator_t() = default;
    locking_iterator_t(base_index_t* se_idx, T_index_iter iter)
        : se_idx(se_idx), index_iter(iter)
    {
        se_idx->get_lock().lock_shared();
    }
    locking_iterator_t(const locking_iterator_t& other)
        : se_idx(other.se_idx), index_iter(other.index_iter)
    {
        se_idx->get_lock().lock_shared();
    }

    ~locking_iterator_t()
    {
        se_idx->get_lock().unlock_shared();
    }

    locking_iterator_t& operator++()
    {
        index_iter++;
        return *this;
    }

    locking_iterator_t operator++(int)
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    const typename T_index_iter::value_type& operator*() const
    {
        return *index_iter;
    }

    const typename T_index_iter::value_type* operator->() const
    {
        return &(*index_iter);
    }

    bool operator==(const locking_iterator_t& other) const
    {
        return index_iter == other.index_iter;
    }

    bool operator!=(const locking_iterator_t& other) const
    {
        return !(*this == other);
    }
};

} // namespace index
} // namespace db
} // namespace gaia
