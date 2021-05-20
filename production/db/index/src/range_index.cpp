/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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
    std::lock_guard<std::recursive_mutex> lock(index_lock);
    return range_index_iterator_t(this, index.cbegin(), index.cend());
}

range_index_iterator_t range_index_t::end()
{
    std::lock_guard<std::recursive_mutex> lock(index_lock);
    return range_index_iterator_t(this, index.cend(), index.cend());
}

range_index_iterator_t range_index_t::find(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(index_lock);
    return range_index_iterator_t(this, index.find(key), index.cend());
}

range_index_iterator_t range_index_t::lower_bound(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(index_lock);
    return range_index_iterator_t(this, index.lower_bound(key), index.cend());
}

range_index_iterator_t range_index_t::upper_bound(const index_key_t& key)
{
    std::lock_guard<std::recursive_mutex> lock(index_lock);
    return range_index_iterator_t(this, index.upper_bound(key), index.cend());
}

std::pair<range_index_iterator_t, range_index_iterator_t> range_index_t::equal_range(const index_key_t& key)
{
    return {find(key), upper_bound(key)};
}

} // namespace index
} // namespace db
} // namespace gaia
