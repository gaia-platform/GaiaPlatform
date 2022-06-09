////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

template <typename T_structure>
index_generator_t<T_structure>::index_generator_t(
    std::recursive_mutex& mutex,
    const T_structure& data,
    std::vector<char>&& storage,
    const index_key_t& begin_key,
    const index_key_t& end_key,
    gaia_txn_id_t txn_id)
    : m_initialized(false)
    , m_index_lock(mutex)
    , m_data(data)
    , m_storage(std::move(storage))
    , m_begin_key(begin_key)
    , m_end_key(end_key)
    , m_txn_id(txn_id)
{
}

template <typename T_structure>
std::optional<index_record_t> index_generator_t<T_structure>::operator()()
{
    if (!m_initialized)
    {
        m_index_lock.lock();
        auto iterators = range();
        m_it = iterators.first;
        m_end = iterators.second;
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
