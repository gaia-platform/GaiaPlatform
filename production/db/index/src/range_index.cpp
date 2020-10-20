/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "range_index.hpp"

#include <utility>

namespace gaia
{
namespace db
{
namespace index
{

void range_index_t::insert_index_entry(index_key_t&& key, index_record_t record)
{
    std::lock_guard lock(index_lock);
    index.insert(std::make_pair(key, record));
}

range_index_iterator_t range_index_t::begin()
{
    std::shared_lock lock(index_lock);
    return range_index_iterator_t(this, index.cbegin());
}

range_index_iterator_t range_index_t::end()
{
    std::shared_lock lock(index_lock);
    return range_index_iterator_t(this, index.cend());
}

range_index_iterator_t range_index_t::find(const index_key_t& key)
{
    std::shared_lock lock(index_lock);
    return range_index_iterator_t(this, index.find(key));
}

range_index_iterator_t range_index_t::lower_bound(const index_key_t& key)
{
    std::shared_lock lock(index_lock);
    return range_index_iterator_t(this, index.lower_bound(key));
}

range_index_iterator_t range_index_t::upper_bound(const index_key_t& key)
{
    std::shared_lock lock(index_lock);
    return range_index_iterator_t(this, index.upper_bound(key));
}

std::pair<range_index_iterator_t, range_index_iterator_t> range_index_t::equal_range(const index_key_t& key)
{
    return {find(key), upper_bound(key)};
}

} // namespace index
} // namespace db
} // namespace gaia
