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
    base_index_t* m_db_idx = nullptr;
    T_index_iter m_index_iter;
    T_index_iter m_end_iter;

public:
    locking_iterator_t() = default;
    locking_iterator_t(base_index_t* db_idx, T_index_iter iter, T_index_iter end_iter)
    {
        m_db_idx = db_idx;
        m_index_iter = iter;
        m_end_iter = end_iter;

        db_idx->get_lock().lock();
    }
    locking_iterator_t(const locking_iterator_t& other)
    {
        m_db_idx = other.m_db_idx;
        m_index_iter = other.m_index_iter;
        m_end_iter = other.m_end_iter;

        m_db_idx->get_lock().lock();
    }

    ~locking_iterator_t()
    {
        if (m_db_idx != nullptr)
        {
            m_db_idx->get_lock().unlock();
        }
    }

    locking_iterator_t& operator++()
    {
        m_index_iter++;
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
        auto key = m_index_iter->first;

        do
        {
            ++m_index_iter;
        } while (m_index_iter->first == key && m_index_iter != m_end_iter);

        return *this;
    }
    const typename T_index_iter::value_type& operator*() const
    {
        return *m_index_iter;
    }

    const typename T_index_iter::value_type* operator->() const
    {
        return &(*m_index_iter);
    }

    bool operator==(const locking_iterator_t& other) const
    {
        return m_index_iter == other.m_index_iter;
    }

    bool operator!=(const locking_iterator_t& other) const
    {
        return !(*this == other);
    }
};

} // namespace index
} // namespace db
} // namespace gaia
