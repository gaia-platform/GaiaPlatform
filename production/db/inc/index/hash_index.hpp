/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unordered_map>

#include "index.hpp"
#include "index_iterator.hpp"

namespace gaia
{
namespace db
{
namespace index
{

using hash_type_t = std::unordered_multimap<index_key_t, index_record_t, index_key_hash>;
using hash_index_iterator_t = index_iterator_t<hash_type_t, hash_type_t::const_iterator>;

/**
* Actual hash index implementation.
*/

class hash_index_t : public index_t<hash_type_t, hash_index_iterator_t>
{
public:
    explicit hash_index_t(gaia::common::gaia_id_t index_id)
        : index_t(index_id, catalog::index_type_t::hash)
    {
    }
    ~hash_index_t() = default;

    hash_index_iterator_t begin();
    hash_index_iterator_t end();
    hash_index_iterator_t find(const index_key_t& key);

    std::pair<hash_index_iterator_t, hash_index_iterator_t> equal_range(const index_key_t& key);
};

} // namespace index
} // namespace db
} // namespace gaia
