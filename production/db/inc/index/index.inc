/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Index structure maintenance
template <typename T, typename T_iterator>
void index_t<T, T_iterator>::insert_index_entry(index_key_t&& key, index_record_t record)
{
    std::lock_guard lock(index_lock);
    auto old_it = find_physical_key(key, record);
    if (old_it != index.end())
    {
        // Update old record matching this physical key
        old_it->second = record;
    }
    else
    {
        index.insert(std::make_pair(key, record));
    }
}

// Clear index structure
template <typename T, typename T_iterator>
void index_t<T, T_iterator>::clear()
{
    std::lock_guard lock(index_lock);
    index.clear();
}

// RAII class for bulk index maintenance operations
template <typename T, typename T_iterator>
index_writer_t<T> index_t<T, T_iterator>::get_writer()
{
    return index_writer_t<T>(this, index);
}

template <typename T, typename T_iterator>
std::function<std::optional<index_record_t>()> index_t<T, T_iterator>::generator(gaia_txn_id_t txn_id)
{
    std::unique_lock<std::recursive_mutex> lock(index_lock);
    auto it = index.cbegin();

    return [end_iter = index.cend(), lock = {std::move(lock)}, it, txn_id]() mutable -> std::optional<index_record_t> {
        while (it != end_iter)
        {
            index_record_t rec = it->second;

            if (rec.txn_id > txn_id)
            {
                ++it;
            }
            else
            {
                ++it;
                return rec;
            }
        }
        // Signal end of iteration.
        return std::nullopt;
    };
}

// find physical key corresponding to a logical_key + record or return the end iterator.
template <typename T, typename T_iterator>
auto index_t<T, T_iterator>::find_physical_key(index_key_t& key, index_record_t& record)
{
    auto idx_rec = index.find(key);
    while (idx_rec != index.end() && idx_rec->first == key)
    {
        if (idx_rec->second.locator == record.locator && idx_rec->second.txn_id == record.txn_id)
        {
            return idx_rec;
        }
        ++idx_rec;
    }

    return index.end();
}
