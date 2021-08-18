/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Index structure maintenance.
template <typename T_structure, typename T_iterator>
void index_t<T_structure, T_iterator>::insert_index_entry(index_key_t&& key, index_record_t record)
{
    std::lock_guard lock(m_index_lock);
    auto old_it = find_physical_key(key, record);
    if (old_it != m_data.end())
    {
        // Update old record matching this physical key.
        old_it->second = record;
    }
    else
    {
        m_data.insert(std::make_pair(key, record));
    }
}

// Clear index structure.
template <typename T_structure, typename T_iterator>
void index_t<T_structure, T_iterator>::clear()
{
    std::lock_guard lock(m_index_lock);
    m_data.clear();
}

// RAII class for bulk index maintenance operations.
template <typename T_structure, typename T_iterator>
index_writer_guard_t<T_structure> index_t<T_structure, T_iterator>::get_writer()
{
    return index_writer_guard_t<T_structure>(this, m_data);
}

template <typename T_structure>
index_generator_t<T_structure>::index_generator_t(std::recursive_mutex& mutex, T_structure& data, gaia_txn_id_t txn_id)
    : m_index_lock(mutex), m_it(data.cbegin()), m_end(data.cend()), m_txn_id(txn_id)
{
}

template <typename T_structure>
index_generator_t<T_structure>::index_generator_t(
    std::recursive_mutex& mutex,
    typename T_structure::const_iterator begin,
    typename T_structure::const_iterator end,
    gaia_txn_id_t txn_id)
    : m_index_lock(mutex), m_it(begin), m_end(end), m_txn_id(txn_id)
{
}

template <typename T_structure>
std::optional<index_record_t> index_generator_t<T_structure>::operator()()
{
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

template <typename T_structure, typename T_iterator>
std::shared_ptr<common::iterators::generator_t<index_record_t>> index_t<T_structure, T_iterator>::generator(gaia_txn_id_t txn_id)
{
    return std::make_shared<index_generator_t<T_structure>>(get_lock(), m_data, txn_id);
}

// Find physical key corresponding to a logical_key + record or return the end iterator.
template <typename T_structure, typename T_iterator>
typename T_structure::iterator index_t<T_structure, T_iterator>::find_physical_key(index_key_t& key, index_record_t& record)
{
    auto it = m_data.find(key);
    while (it != m_data.end() && it->first == key)
    {
        if (it->second.locator == record.locator && it->second.txn_id == record.txn_id)
        {
            return it;
        }
        ++it;
    }

    return m_data.end();
}
