/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "hash_index.hpp"

#include "gaia_internal/common/retail_assert.hpp"

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

std::shared_ptr<common::iterators::generator_t<index_record_t>> hash_index_t::equal_range_generator(gaia_txn_id_t txn_id, const index_key_t& key)
{
    return std::make_shared<index_generator_t<hash_type_t>>(get_lock(), m_data, key, key, txn_id);
}

template <>
std::pair<hash_type_t::iterator, hash_type_t::iterator>
index_writer_guard_t<hash_type_t>::equal_range(const index_key_t& key)
{
    return m_data.equal_range(key);
}

template <>
std::pair<hash_type_t::const_iterator, hash_type_t::const_iterator>
index_generator_t<hash_type_t>::range(const index_key_t& begin_key, const index_key_t& end_key) const
{
    if (begin_key.size() == 0 && end_key.size() == 0)
    {
        return {m_data.cbegin(), m_data.cend()};
    }
    else if (begin_key == end_key)
    {
        return m_data.equal_range(begin_key);
    }
    else
    {
        ASSERT_UNREACHABLE("hash index does not support non-equal range queries.");
    }
}

} // namespace index
} // namespace db
} // namespace gaia
