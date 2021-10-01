/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

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

template <typename T_structure, typename T_iterator>
std::shared_ptr<common::iterators::generator_t<index_record_t>> index_t<T_structure, T_iterator>::generator(
    gaia_txn_id_t txn_id)
{
    return std::make_shared<index_generator_t<T_structure>>(get_lock(), m_data, txn_id);
}

// Find physical key corresponding to a logical_key + record or return the end iterator.
template <typename T_structure, typename T_iterator>
typename T_structure::iterator index_t<T_structure, T_iterator>::find_physical_key(
    index_key_t& key, index_record_t& record)
{
    for (auto it = m_data.find(key);
         it != m_data.end() && it->first == key;
         ++it)
    {
        if (it->second.locator == record.locator && it->second.txn_id == record.txn_id)
        {
            return it;
        }
    }

    return m_data.end();
}
