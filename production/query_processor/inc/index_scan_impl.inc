/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

bool base_index_scan_impl_t::is_visible(const db::index::index_record_t& rec)
{
    return db::locator_exists(rec.locator) && db::locator_to_offset(rec.locator) == rec.offset;
}

template <typename T_index, typename T_iter>
void index_scan_impl_t<T_index, T_iter>::init()
{
    auto rec_generator = db::client_t::get_record_generator_for_index(m_index_id, physical_operator_t::get_current_txn_id());

    // Only return visible records from remote.
    m_remote_iter = remote_scan_iterator_t(
        rec_generator,
        is_visible);

    auto idx_iter = db::get_indexes()->find(m_index_id);

    ASSERT_PRECONDITION(idx_iter != db::get_indexes()->end(), "Index does not exist.");

    m_index = idx_iter->second;

    m_local_iter = static_cast<T_index*>(m_index.get())->begin();
    m_local_iter_end = static_cast<T_index*>(m_index.get())->end();

    // Advance local iterator to first visible record.
    if (!local_end() && !is_visible(m_local_iter->second))
        advance_local();

    // Assign the first visible locator.
    next_visible_locator();
    m_initialized = true;
}

template <typename T_index, typename T_iter>
db::gaia_locator_t index_scan_impl_t<T_index, T_iter>::locator()
{
    if (!m_initialized)
    {
        init();
    }
    return m_locator;
}

template <typename T_index, typename T_iter>
void index_scan_impl_t<T_index, T_iter>::advance_local()
{
    do
    {
        ++m_local_iter;
    } while (!local_end() && !is_visible(m_local_iter->second));
}

template <typename T_index, typename T_iter>
bool index_scan_impl_t<T_index, T_iter>::local_end() const
{
    return m_local_iter == m_local_iter_end;
}

template <typename T_index, typename T_iter>
void index_scan_impl_t<T_index, T_iter>::advance_remote()
{
    if (!remote_end())
    {
        ++m_remote_iter;
    }
}

template <typename T_index, typename T_iter>
bool index_scan_impl_t<T_index, T_iter>::remote_end() const
{
    return !static_cast<bool>(m_remote_iter);
}

// Ordering matters for range iterators.
template <>
bool index_scan_impl_t<db::index::range_index_t, db::index::range_index_iterator_t>::select_local_for_merge() const
{
    return record_to_key(m_local_iter->second) <= record_to_key(*m_remote_iter);
}

// Ignore ordering for other iterators.
template <typename T_index, typename T_iter>
bool index_scan_impl_t<T_index, T_iter>::select_local_for_merge() const
{
    return true;
}

template <typename T_index, typename T_iter>
bool index_scan_impl_t<T_index, T_iter>::has_more()
{
    if (!m_initialized)
    {
        init();
    }
    return !remote_end() && !local_end();
}

template <typename T_index, typename T_iter>
db::index::index_key_t index_scan_impl_t<T_index, T_iter>::record_to_key(const db::index::index_record_t& record) const
{
    auto obj = db::locator_to_ptr(record.locator);
    return db::index::index_builder_t::make_key(m_index_id, obj->type, reinterpret_cast<const uint8_t*>(obj->payload));
}

template <typename T_index, typename T_iter>
void index_scan_impl_t<T_index, T_iter>::next_visible_locator()
{
    if (!local_end() && !remote_end())
    {
        if (select_local_for_merge())
        {
            m_locator = m_local_iter->second.locator;
            advance_local();
        }
        else
        {
            m_locator = m_remote_iter->locator;
            advance_remote();
        }
    }
    else if (local_end() && !remote_end())
    {
        m_locator = m_remote_iter->locator;
        advance_remote();
    }
    else if (remote_end() && !local_end())
    {
        m_locator = m_local_iter->second.locator;
        advance_local();
    }
    else // No visible record here.
    {
        m_locator = db::c_invalid_gaia_locator;
    }
}
