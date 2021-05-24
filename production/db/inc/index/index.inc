/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename... T_keys>
index_key_t::index_key_t(T_keys... keys)
{
    multi_insert(keys...);
}

template <typename T_key, typename... T_keys>
void index_key_t::multi_insert(T_key key_value, T_keys... rest)
{
    insert(key_value);
    multi_insert(rest...);
}

template <typename T_key>
void index_key_t::multi_insert(T_key key_value)
{
    insert(key_value);
}

// Index structure maintenance
template <typename T_structure, typename T_iterator>
void index_t<T_structure, T_iterator>::insert_index_entry(index_key_t&& key, index_record_t record)
{
    std::lock_guard lock(m_index_lock);
    auto old_it = find_physical_key(key, record);
    if (old_it != m_index.end())
    {
        // Update old record matching this physical key
        old_it->second = record;
    }
    else
    {
        m_index.insert(std::make_pair(key, record));
    }
}

// Clear index structure
template <typename T_structure, typename T_iterator>
void index_t<T_structure, T_iterator>::clear()
{
    std::lock_guard lock(m_index_lock);
    m_index.clear();
}

// RAII class for bulk index maintenance operations
template <typename T_structure, typename T_iterator>
index_writer_t<T_structure> index_t<T_structure, T_iterator>::get_writer()
{
    return index_writer_t<T_structure>(this, m_index);
}

template <typename T_structure, typename T_iterator>
std::function<std::optional<index_record_t>()> index_t<T_structure, T_iterator>::generator(gaia_txn_id_t txn_id)
{
    std::unique_lock<std::recursive_mutex> lock(m_index_lock);
    auto it = m_index.cbegin();

    return [end_iter = m_index.cend(), lock = {std::move(lock)}, it, txn_id]() mutable -> std::optional<index_record_t> {
        while (it != end_iter)
        {
            index_record_t record = it->second;
            ++it;

            if (record.txn.id <= txn_id)
            {
                return record;
            }
        }
        // Signal end of iteration.
        return std::nullopt;
    };
}

// Find physical key corresponding to a logical_key + record or return the end iterator.

template <typename T_structure, typename T_iterator>
auto index_t<T_structure, T_iterator>::find_physical_key(index_key_t& key, index_record_t& record)
{
    auto it = m_index.find(key);
    while (it != m_index.end() && it->first == key)
    {
        if (it->second.locator == record.locator && it->second.txn_id == record.txn_id)
        {
            return it;
        }
        ++it;
    }

    return m_index.end();
}
