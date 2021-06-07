/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "hash_index.hpp"

#include <utility>

#include "txn_metadata.hpp"

namespace gaia
{
namespace db
{
namespace index
{

hash_index_iterator_t hash_index_t::begin()
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return hash_index_iterator_t(this, m_data.cbegin(), m_data.cend());
}

hash_index_iterator_t hash_index_t::end()
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return hash_index_iterator_t(this, m_data.cend(), m_data.cend());
}

hash_index_iterator_t hash_index_t::find(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return hash_index_iterator_t(this, m_data.find(key), m_data.cend());
}

std::pair<hash_index_iterator_t, hash_index_iterator_t> hash_index_t::equal_range(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    const auto [start, end] = m_data.equal_range(key);

    return std::make_pair(hash_index_iterator_t(this, start, m_data.cend()), hash_index_iterator_t(this, end, m_data.cend()));
}

} // namespace index
} // namespace db
} // namespace gaia
