/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "index.hpp"

#include <string>

#include "gaia_internal/common/retail_assert.hpp"

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
    return compare(other) == 0;
}

void index_key_t::insert(gaia::db::payload_types::data_holder_t value)
{
    m_key_values.push_back(value);
}

std::size_t index_key_t::size() const
{
    return m_key_values.size();
}

/*
* Combine hash of all data holders in this key.
* Concatenate all hash values and rehash.
*/
std::size_t index_key_hash::operator()(index_key_t const& key) const
{
<<<<<<< HEAD
    size_t seed = c_default_seed;

    /*
    * Combine hash values for each key.
    * The algorithm here is same as boost::hash_combine.
    * Taken from http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.18.2680
    */
=======
    std::ostringstream hash_concat;
>>>>>>> 6d0f1faf4... Changed the hash combiner and addressed more comments
    for (payload_types::data_holder_t data : key.m_key_values)
    {
        hash_concat << data.hash();
    }

    return std::hash<std::string>{}(hash_concat.str());
}

/*
 * ostream operator overloads
 */

std::ostream& operator<<(std::ostream& os, const index_record_t& record)
{
    os << "locator: "
       << record.locator
       << "\ttxn_id: "
       << record.txn_id
       << "\toffset: "
       << record.offset
       << "\tdeleted: "
       << record.deleted
       << std::endl;
    return os;
}

std::ostream& operator<<(std::ostream& os, const index_key_t& key)
{
    for (const auto& k : key.m_key_values)
    {
        os << k << " ";
    }
    return os;
}

} // namespace index
} // namespace db
} // namespace gaia
