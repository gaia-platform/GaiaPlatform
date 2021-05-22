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
    return hash_index_iterator_t(this, m_index.cbegin(), m_index.cend());
}

hash_index_iterator_t hash_index_t::end()
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return hash_index_iterator_t(this, m_index.cend(), m_index.cend());
}

hash_index_iterator_t hash_index_t::find(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return hash_index_iterator_t(this, m_index.find(key), m_index.cend());
}

std::pair<hash_index_iterator_t, hash_index_iterator_t> hash_index_t::equal_range(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    const auto [start, end] = m_index.equal_range(key);

    return std::make_pair(hash_index_iterator_t(this, start, m_index.cend()), hash_index_iterator_t(this, end, m_index.cend()));
}

} // namespace index
} // namespace db
} // namespace gaia
