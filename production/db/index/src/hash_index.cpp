/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "hash_index.hpp"

#include <utility>

namespace gaia
{
namespace db
{
namespace index
{

void hash_index_t::insert_index_entry(index_key_t&& key, index_record_t record)
{
    std::lock_guard lock(index_lock);
    index.insert(std::make_pair(key, record));
}

hash_index_iterator_t hash_index_t::begin()
{
    std::shared_lock lock(index_lock);
    return hash_index_iterator_t(this, index.cbegin());
}

hash_index_iterator_t hash_index_t::end()
{
    std::shared_lock lock(index_lock);
    return hash_index_iterator_t(this, index.cend());
}

hash_index_iterator_t hash_index_t::find(const index_key_t& key)
{
    std::shared_lock lock(index_lock);
    return hash_index_iterator_t(this, index.find(key));
}

std::pair<hash_index_iterator_t, hash_index_iterator_t> hash_index_t::equal_range(const index_key_t& key)
{
    std::shared_lock lock(index_lock);
    const auto [start, end] = index.equal_range(key);

    return std::make_pair(hash_index_iterator_t(this, start), hash_index_iterator_t(this, end));
}

} // namespace index
} // namespace db
} // namespace gaia
