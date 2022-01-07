////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "range_index.hpp"

#include <utility>

#include "txn_metadata.hpp"

namespace gaia
{
namespace db
{
namespace index
{

range_index_iterator_t range_index_t::begin()
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return range_index_iterator_t(this, m_data.cbegin(), m_data.cend());
}

range_index_iterator_t range_index_t::end()
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return range_index_iterator_t(this, m_data.cend(), m_data.cend());
}

range_index_iterator_t range_index_t::find(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return range_index_iterator_t(this, m_data.find(key), m_data.cend());
}

range_index_iterator_t range_index_t::lower_bound(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return range_index_iterator_t(this, m_data.lower_bound(key), m_data.cend());
}

range_index_iterator_t range_index_t::upper_bound(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    return range_index_iterator_t(this, m_data.upper_bound(key), m_data.cend());
}

std::pair<range_index_iterator_t, range_index_iterator_t> range_index_t::equal_range(const index_key_t& key)
{
    auto first = find(key);
    auto last = (first == end()) ? end() : upper_bound(key);
    return {first, last};
}

std::shared_ptr<common::iterators::generator_t<index_record_t>> range_index_t::equal_range_generator(
    gaia_txn_id_t txn_id,
    const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);
    auto first = m_data.find(key);
    auto last = (first == m_data.cend()) ? m_data.cend() : m_data.upper_bound(key);
    return std::make_shared<index_generator_t<range_type_t>>(get_lock(), first, last, txn_id);
}

std::shared_ptr<common::iterators::generator_t<index_record_t>> range_index_t::range_generator(
    gaia_txn_id_t txn_id,
    std::optional<index_key_t> lower_bound,
    std::optional<index_key_t> upper_bound)
{
    std::lock_guard<std::recursive_mutex> lock(m_index_lock);

    range_type_t::const_iterator lower_iter = (lower_bound.has_value()) ? m_data.lower_bound(*lower_bound) : m_data.cbegin();
    range_type_t::const_iterator upper_iter = (upper_bound.has_value()) ? m_data.upper_bound(*upper_bound) : m_data.cend();

    return std::make_shared<index_generator_t<range_type_t>>(get_lock(), lower_iter, upper_iter, txn_id);
}

template <>
std::pair<range_type_t::iterator, range_type_t::iterator>
index_writer_guard_t<range_type_t>::equal_range(const index_key_t& key)
{
    auto first = m_data.find(key);
    auto last = (first == m_data.end()) ? m_data.end() : m_data.upper_bound(key);
    return {first, last};
}

template <>
std::pair<range_type_t::const_iterator, range_type_t::const_iterator>
index_generator_t<range_type_t>::range() const
{
    range_type_t::const_iterator first = (m_begin_key == c_unbound_index_key) ? m_data.cbegin() : m_data.find(m_begin_key);
    range_type_t::const_iterator last = (first == m_data.cend() || m_end_key == c_unbound_index_key) ? m_data.cend() : m_data.upper_bound(m_end_key);
    return {first, last};
}

} // namespace index
} // namespace db
} // namespace gaia
