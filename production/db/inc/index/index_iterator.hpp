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

template <typename T_index, typename T_index_iter>
class locking_iterator_t
{
private:
    base_index_t* db_idx;
    T_index_iter index_iter;
    T_index_iter end_iter;

public:
    locking_iterator_t() = default;
    locking_iterator_t(base_index_t* db_idx, T_index_iter iter, T_index_iter end_iter)
    {
        this->db_idx = db_idx;
        this->index_iter = iter;
        this->end_iter = end_iter;

        db_idx->get_lock().lock_shared();
    }
    locking_iterator_t(const locking_iterator_t& other)
    {
        this->db_idx = other.db_idx;
        this->index_iter = other.index_iter;
        this->end_iter = other.end_iter;

        db_idx->get_lock().lock_shared();
    }

    ~locking_iterator_t()
    {
        db_idx->get_lock().unlock_shared();
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

    locking_iterator_t& next_key() const
    {
        auto key = index_iter->first;

        do
        {
            ++index_iter;
        } while (index_iter->first == key && index_iter != end_iter);

        return *this;
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
