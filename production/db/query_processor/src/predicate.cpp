////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "predicate.hpp"

#include "gaia_internal/db/index_builder.hpp"

#include "db_shared_data.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace query_processor
{
namespace scan
{

bool index_predicate_t::filter(const gaia_ptr_t&) const
{
    return true;
}

gaia::db::messages::index_query_t index_predicate_t::query_type() const
{
    return messages::index_query_t::NONE;
}

serialized_index_query_t index_predicate_t::as_query(common::gaia_id_t, flatbuffers::FlatBufferBuilder&) const
{
    return c_null_predicate;
}

index_point_read_predicate_t::index_point_read_predicate_t(index::index_key_t index_key)
    : m_key(index_key)
{
}

index_point_read_predicate_t::index_point_read_predicate_t(index::index_key_t&& index_key)
    : m_key(index_key)
{
}

gaia::db::messages::index_query_t index_point_read_predicate_t::query_type() const
{
    return messages::index_query_t::index_point_read_query_t;
}

serialized_index_query_t index_point_read_predicate_t::as_query(common::gaia_id_t index_id, flatbuffers::FlatBufferBuilder& builder) const
{
    payload_types::data_write_buffer_t buffer(builder);
    index::index_builder_t::serialize_key(index_id, m_key, buffer);
    auto output = buffer.output();

    auto query = messages::Createindex_point_read_query_t(builder, output);

    return query.Union();
}

index_equal_range_predicate_t::index_equal_range_predicate_t(index::index_key_t index_key)
    : m_key(index_key)
{
}

index_equal_range_predicate_t::index_equal_range_predicate_t(index::index_key_t&& index_key)
    : m_key(index_key)
{
}

gaia::db::messages::index_query_t index_equal_range_predicate_t::query_type() const
{
    return messages::index_query_t::index_equal_range_query_t;
}

serialized_index_query_t index_equal_range_predicate_t::as_query(common::gaia_id_t index_id, flatbuffers::FlatBufferBuilder& builder) const
{
    payload_types::data_write_buffer_t buffer(builder);
    index::index_builder_t::serialize_key(index_id, m_key, buffer);
    auto output = buffer.output();

    auto query = messages::Createindex_equal_range_query_t(builder, output);

    return query.Union();
}

const index::index_key_t& index_equal_range_predicate_t::key() const
{
    return m_key;
}

const index::index_key_t& index_point_read_predicate_t::key() const
{
    return m_key;
}

range_bound_t::range_bound_t(std::optional<index::index_key_t> index_key, bool is_inclusive)
    : m_key(index_key), m_is_inclusive(is_inclusive)
{
}

const std::optional<index::index_key_t>& range_bound_t::key() const
{
    return m_key;
}
bool range_bound_t::is_inclusive() const
{
    return m_is_inclusive;
}

index_range_predicate_t::index_range_predicate_t(gaia_id_t index_id, range_bound_t lower_bound, range_bound_t upper_bound)
    : m_index_id(index_id), m_lower_bound(lower_bound), m_upper_bound(upper_bound)
{
}

const range_bound_t& index_range_predicate_t::lower_bound() const
{
    return m_lower_bound;
}

const range_bound_t& index_range_predicate_t::upper_bound() const
{
    return m_upper_bound;
}

gaia::db::messages::index_query_t index_range_predicate_t::query_type() const
{
    return messages::index_query_t::index_range_query_t;
}

serialized_index_query_t index_range_predicate_t::as_query(flatbuffers::FlatBufferBuilder& builder) const
{
    payload_types::serialization_output_t lower_bound = 0;
    payload_types::serialization_output_t upper_bound = 0;

    payload_types::data_write_buffer_t buffer(builder);
    if (m_lower_bound.key())
    {
        index::index_builder_t::serialize_key(*m_lower_bound.key(), buffer);
        lower_bound = buffer.output();
    }

    if (m_upper_bound.key())
    {
        index::index_builder_t::serialize_key(*m_upper_bound.key(), buffer);
        upper_bound = buffer.output();
    }

    auto query = messages::Createindex_range_query_t(builder, lower_bound, upper_bound);

    return query.Union();
}

bool index_range_predicate_t::filter(const gaia_ptr_t& ptr) const
{
    auto it = get_indexes()->find(m_index_id);
    ASSERT_INVARIANT(it != get_indexes()->end(), "Index structure could not be found.");
    index::db_index_t index = it->second;

    auto key = index::index_builder_t::make_key(index, reinterpret_cast<const uint8_t*>(ptr.data()));

    // Filter out the equal values if bounds are not inclusive.
    if (m_lower_bound.key() && !m_lower_bound.is_inclusive() && *m_lower_bound.key() == key)
    {
        return false;
    }

    if (m_upper_bound.key() && !m_upper_bound.is_inclusive() && *m_upper_bound.key() == key)
    {
        return false;
    }

    return true;
}

} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
