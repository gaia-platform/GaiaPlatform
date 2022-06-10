////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "index.hpp"

#include <string_view>

#include "gaia_internal/common/assert.hpp"

#include "field_access.hpp"
#include "index_iterator.hpp"

using namespace gaia::common;
using namespace gaia::db::index;

namespace gaia
{
namespace db
{
namespace index
{

index_key_t::index_key_t(const index_key_schema_t& key_schema, const uint8_t* payload)
{
    for (field_position_t pos : key_schema.field_positions)
    {
        insert(payload_types::get_field_value(payload, key_schema.binary_schema->data(), pos));
    }
}

int index_key_t::compare(const index_key_t& other) const
{
    ASSERT_PRECONDITION(
        m_key_values.size() == other.m_key_values.size(),
        "An attempt was made to compare index records of different value.");

    int comparison_result = 0;
    for (size_t i = 0; i < m_key_values.size(); i++)
    {
        comparison_result = m_key_values[i].compare(other.m_key_values[i]);

        // Stop scanning once first difference is encountered.
        if (comparison_result != 0)
        {
            return comparison_result;
        }
    }

    ASSERT_INVARIANT(comparison_result == 0, "Internal error: code should have returned already!");

    return comparison_result;
}

bool index_key_t::operator<(const index_key_t& other) const
{
    return compare(other) < 0;
}

bool index_key_t::operator<=(const index_key_t& other) const
{
    return compare(other) <= 0;
}

bool index_key_t::operator>(const index_key_t& other) const
{
    return compare(other) > 0;
}

bool index_key_t::operator>=(const index_key_t& other) const
{
    return compare(other) >= 0;
}

bool index_key_t::operator==(const index_key_t& other) const
{
    if (empty() && other.empty())
    {
        return true;
    }
    else if (empty() || other.empty())
    {
        return false;
    }
    else
    {
        return compare(other) == 0;
    }
}

void index_key_t::insert(gaia::db::payload_types::data_holder_t value)
{
    m_key_values.push_back(value);
}

std::size_t index_key_t::size() const
{
    return m_key_values.size();
}

bool index_key_t::empty() const
{
    return m_key_values.size() == 0;
}

bool index_key_t::is_null() const
{
    for (const auto& value : m_key_values)
    {
        if (!value.is_null)
        {
            return false;
        }
    }
    return true;
}

const std::vector<gaia::db::payload_types::data_holder_t>& index_key_t::values() const
{
    return m_key_values;
}

/**
 * Combine hash of all data holders in this key.
 * Repeatedly concatenate hash values and rehash.
 */
gaia::db::payload_types::data_hash_t index_key_hash::operator()(index_key_t const& key) const
{
    constexpr size_t c_hash_concat_buffer_elems = 2;
    constexpr size_t c_hash_seed = 0x9e3779b9;

    std::size_t hash_concat[c_hash_concat_buffer_elems]{0};
    std::string_view hash_view{reinterpret_cast<const char*>(hash_concat), sizeof(hash_concat)};
    std::size_t prev_hash = c_hash_seed;

    for (payload_types::data_holder_t data : key.m_key_values)
    {
        hash_concat[0] = prev_hash;
        hash_concat[1] = data.hash();
        prev_hash = std::hash<std::string_view>{}(hash_view);
    }

    return prev_hash;
}

/**
 * ostream operator overloads
 */

std::ostream& operator<<(std::ostream& os, const index_record_t& record)
{
    os << "[txn_id: "
       << record.txn_id
       << " locator: "
       << record.locator
       << " offset: "
       << record.offset
       << " operation: "
       << get_enum_value(record.operation)
       << " flags: "
       << record.flags
       << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const index_key_t& key)
{
    for (const auto& key_value : key.m_key_values)
    {
        os << "{" << key_value << "}";
    }
    return os;
}

} // namespace index
} // namespace db
} // namespace gaia
