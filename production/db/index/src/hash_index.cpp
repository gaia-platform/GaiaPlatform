/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "hash_index.hpp"

#include "gaia_internal/common/retail_assert.hpp"

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

template <>
std::pair<hash_type_t::iterator, hash_type_t::iterator>
index_writer_guard_t<hash_type_t>::equal_range(const index_key_t& key)
{
    return m_data.equal_range(key);
}

template <>
std::pair<hash_type_t::const_iterator, hash_type_t::const_iterator>
index_generator_t<hash_type_t>::range() const
{
    if (m_begin_key == c_unbound_index_key && m_end_key == c_unbound_index_key)
    {
        return {m_data.cbegin(), m_data.cend()};
    }
    else if (m_begin_key == m_end_key)
    {
        return m_data.equal_range(m_begin_key);
    }
    else
    {
        ASSERT_UNREACHABLE("Hash index does not support non-equal range queries.");
    }
}

} // namespace index
} // namespace db
} // namespace gaia
