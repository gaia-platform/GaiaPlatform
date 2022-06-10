////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <iterator>
#include <map>
#include <tuple>

#include "index.hpp"
#include "index_iterator.hpp"

namespace gaia
{
namespace db
{
namespace index
{

using range_type_t = std::multimap<index_key_t, index_record_t>;
using range_index_iterator_t = index_iterator_t<range_type_t, range_type_t::const_iterator>;

/**
 * Actual range index implementation.
 */
class range_index_t : public index_t<range_type_t, range_index_iterator_t>
{
public:
    range_index_t(gaia::common::gaia_id_t index_id, index_key_schema_t key_schema, bool is_unique = false)
        : index_t(index_id, catalog::index_type_t::range, key_schema, is_unique)
    {
    }
    ~range_index_t() = default;

    range_index_iterator_t begin() override;
    range_index_iterator_t end() override;

    range_index_iterator_t find(const index_key_t& key) override;
    range_index_iterator_t lower_bound(const index_key_t& key);
    range_index_iterator_t upper_bound(const index_key_t& key);

    std::pair<range_index_iterator_t, range_index_iterator_t> equal_range(const index_key_t& key) override;
};

} // namespace index
} // namespace db
} // namespace gaia
