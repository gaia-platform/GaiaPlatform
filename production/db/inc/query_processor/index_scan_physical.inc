/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Checks visibility of the index entry.
// For now, we always add an index entry when the indexed object is updated
// regardless if the indexed fields are updated. The following logic is sufficient.
bool base_index_scan_physical_t::is_visible(const db::index::index_record_t& rec)
{
    return db::locator_exists(rec.locator) && db::locator_to_offset(rec.locator) == rec.offset;
}

// Helper to get iterators for querying client-side indexes
template <typename T_index, typename T_index_iterator>
std::pair<T_index_iterator, T_index_iterator>
local_iterator_helper(T_index* index, index_predicate_t& predicate)
{
    std::pair<T_index_iterator, T_index_iterator> retval;

    switch (predicate.query_type())
    {
    case messages::index_query_t::NONE:
        retval.first = index->begin();
        retval.second = index->end();
        break;
    case messages::index_query_t::index_equal_range_query_t:
    case messages::index_query_t::index_point_read_query_t:
        retval = index->equal_range(predicate.key());
        break;
    default:
        ASSERT_UNREACHABLE("Unknown query type.");
    }

    return retval;
}

// This method initializes the physical index scan.
// Initialization is fairly expensive and complicated (RPC, locking on structures in the server)
// So we delay the initialization until it is needed.
template <typename T_index, typename T_index_iterator>
void index_scan_physical_t<T_index, T_index_iterator>::init()
{
    auto rec_generator = scan_generator_t::get_record_generator_for_index(m_index_id, db_client_proxy_t::get_current_txn_id(), m_predicate);

    // Only return visible records from remote.
    m_remote_it = remote_scan_iterator_t(
        rec_generator,
        is_visible);

    auto idx_iter = db::get_indexes()->find(m_index_id);

    ASSERT_PRECONDITION(idx_iter != db::get_indexes()->end(), "Index does not exist.");

    m_index = idx_iter->second;

    if (m_predicate)
    {
        auto range = local_iterator_helper<T_index, T_index_iterator>(static_cast<T_index*>(m_index.get()), *m_predicate);
        m_local_it = range.first;
        m_local_it_end = range.second;
    }
    else
    {
        m_local_it = static_cast<T_index*>(m_index.get())->begin();
        m_local_it_end = static_cast<T_index*>(m_index.get())->end();
    }

    // Advance local iterator to first visible record.
    if (!local_end() && !is_visible(m_local_it->second))
        advance_local();

    // Assign the first visible locator.
    next_visible_locator();
    m_initialized = true;
}

template <typename T_index, typename T_index_iterator>
db::gaia_locator_t index_scan_physical_t<T_index, T_index_iterator>::locator()
{
    if (!m_initialized)
    {
        init();
    }
    return m_locator;
}

template <typename T_index, typename T_index_iterator>
void index_scan_physical_t<T_index, T_index_iterator>::advance_local()
{
    do
    {
        ++m_local_it;
    } while (!local_end() && !is_visible(m_local_it->second));
}

template <typename T_index, typename T_index_iterator>
bool index_scan_physical_t<T_index, T_index_iterator>::local_end() const
{
    return m_local_it == m_local_it_end;
}

template <typename T_index, typename T_index_iterator>
void index_scan_physical_t<T_index, T_index_iterator>::advance_remote()
{
    if (!remote_end())
    {
        ++m_remote_it;
    }
}

template <typename T_index, typename T_index_iterator>
bool index_scan_physical_t<T_index, T_index_iterator>::remote_end() const
{
    return !static_cast<bool>(m_remote_it);
}

// Ordering matters for range iterators so we compare the keys between local/remote.
template <>
bool index_scan_physical_t<db::index::range_index_t, db::index::range_index_iterator_t>::should_merge_local() const
{
    return record_to_key(m_local_it->second) <= record_to_key(*m_remote_it);
}

// Ignore ordering for other non-range iterators.
// We exhaust the local changes before pulling in the remote changes.
template <typename T_index, typename T_index_iterator>
bool index_scan_physical_t<T_index, T_index_iterator>::should_merge_local() const
{
    return true;
}

template <typename T_index, typename T_index_iterator>
bool index_scan_physical_t<T_index, T_index_iterator>::has_more()
{
    if (!m_initialized)
    {
        init();
    }

    return !remote_end() || !local_end();
}

template <typename T_index, typename T_index_iterator>
db::index::index_key_t index_scan_physical_t<T_index, T_index_iterator>::record_to_key(const db::index::index_record_t& record) const
{
    auto obj = db::locator_to_ptr(record.locator);
    return db::index::index_builder_t::make_key(m_index_id, obj->type, reinterpret_cast<const uint8_t*>(obj->data()));
}

template <typename T_index, typename T_index_iterator>
void index_scan_physical_t<T_index, T_index_iterator>::next_visible_locator()
{
    if (!local_end() && !remote_end())
    {
        if (should_merge_local())
        {
            m_locator = m_local_it->second.locator;
            advance_local();
        }
        else
        {
            m_locator = m_remote_it->locator;
            advance_remote();
        }
    }
    else if (local_end() && !remote_end())
    {
        m_locator = m_remote_it->locator;
        advance_remote();
    }
    else if (remote_end() && !local_end())
    {
        m_locator = m_local_it->second.locator;
        advance_local();
    }
    else // No visible record here.
    {
        m_locator = db::c_invalid_gaia_locator;
    }
}
