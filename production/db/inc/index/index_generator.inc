/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

template <typename T_structure>
index_generator_t<T_structure>::index_generator_t(std::recursive_mutex& mutex, T_structure& data, gaia_txn_id_t txn_id)
    : m_initialized(false), m_index_lock(mutex), m_it(data.cbegin()), m_end(data.cend()), m_txn_id(txn_id)
{
}

template <typename T_structure>
index_generator_t<T_structure>::index_generator_t(
    std::recursive_mutex& mutex,
    typename T_structure::const_iterator begin,
    typename T_structure::const_iterator end,
    gaia_txn_id_t txn_id)
    : m_initialized(false), m_index_lock(mutex), m_it(begin), m_end(end), m_txn_id(txn_id)
{
}

template <typename T_structure>
std::optional<index_record_t> index_generator_t<T_structure>::operator()()
{
    if (!m_initialized)
    {
        m_index_lock.lock();
        m_initialized = true;
    }

    while (m_it != m_end)
    {
        index_record_t record = m_it->second;
        ++m_it;

        if (record.txn_id <= m_txn_id)
        {
            return record;
        }
    }
    // Signal end of iteration.
    return std::nullopt;
}

template <typename T_structure>
void index_generator_t<T_structure>::cleanup()
{
    if (m_initialized)
    {
        m_index_lock.unlock();
    }
}
