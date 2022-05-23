////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

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
    hash_index_t(gaia::common::gaia_id_t index_id, index_key_schema_t key_schema, bool is_unique = false)
        : index_t(index_id, catalog::index_type_t::hash, key_schema, is_unique)
    {
    }
    ~hash_index_t() = default;

    hash_index_iterator_t begin() override;
    hash_index_iterator_t end() override;
    hash_index_iterator_t find(const index_key_t& key) override;

    std::pair<hash_index_iterator_t, hash_index_iterator_t> equal_range(const index_key_t& key) override;
};

} // namespace index
} // namespace db
} // namespace gaia
