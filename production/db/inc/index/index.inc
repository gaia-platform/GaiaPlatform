/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Index structure maintenance.

template <typename T_structure, typename T_iterator>
void index_t<T_structure, T_iterator>::insert_index_entry(index_key_t&& key, index_record_t record)
{
    std::lock_guard lock(m_index_lock);

    // Update the last updated txn id if necessary.
    if (record.txn_id > m_last_updated_txn_id)
    {
        m_last_updated_txn_id = record.txn_id;
    }

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

template <typename T_structure, typename T_iterator>
void index_t<T_structure, T_iterator>::remove_index_entry_with_offsets(const std::array<gaia_offset_t, c_offset_buffer_size>& offsets, gaia_txn_id_t gc_txn_id)
{
    std::lock_guard lock(m_index_lock);

    if (gc_txn_id > m_last_updated_txn_id)
    {
        // Nothing to do here. We have not made any updates to this index past the gc point, so the offsets associated with
        // the gc_txn_id do not exist in this index.
        return;
    }

    for (auto it = m_data.begin(); it != m_data.end();)
    {
        if (it->second.txn_id <= gc_txn_id
            && std::find(offsets.begin(), offsets.end(), it->second.offset) != offsets.end())
        {
            it = m_data.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

template <typename T_structure, typename T_iterator>
void index_t<T_structure, T_iterator>::mark_entries_committed(gaia_txn_id_t metadata_truncation_txn_id)
{
    {
        std::lock_guard lock(m_index_lock);
        if (m_last_mark_committed_txn_id >= m_last_updated_txn_id)
        {
            // Nothing to do here. All index entries have already been marked and we have not modified the index since.
            //
            // It is unsafe to check m_last_updated_txn_id against the metadata truncation id since metadata_truncation_txn_id can advance
            // ahead of the last mark committed point without updating the index. This situation leads to required lookups
            // on truncated metadata on the next index lookup.
            return;
        }
    }

    for (auto it = m_data.begin(); it != m_data.end(); ++it)
    {
        auto& record = it->second;

        if (record.txn_id <= metadata_truncation_txn_id)
        {
            // This should ideally be atomic, but the records stop becoming copyable.
            std::lock_guard lock(m_index_lock);
            mark_committed(record);
        }
    }

    {
        std::lock_guard lock(m_index_lock);
        if (metadata_truncation_txn_id > m_last_mark_committed_txn_id)
        {
            m_last_mark_committed_txn_id = metadata_truncation_txn_id;
        }
    }
}

// Clear index structure.
template <typename T_structure, typename T_iterator>
void index_t<T_structure, T_iterator>::clear()
{
    std::lock_guard lock(m_index_lock);
    m_data.clear();
    m_last_updated_txn_id = c_invalid_gaia_txn_id;
    m_last_mark_committed_txn_id = c_invalid_gaia_txn_id;
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
    const index_key_t& key, const index_record_t& record)
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
