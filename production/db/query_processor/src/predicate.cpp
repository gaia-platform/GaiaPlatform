////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "predicate.hpp"

#include "gaia_internal/db/index_builder.hpp"

namespace gaia
{
namespace db
{
namespace query_processor
{
namespace scan
{

index_predicate_t::index_predicate_t(index::index_key_t index_key)
    : m_key(std::move(index_key))
{
}

index_predicate_t::index_predicate_t(index::index_key_t&& index_key)
    : m_key(index_key)
{
}

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
    : index_predicate_t(index_key)
{
}

index_point_read_predicate_t::index_point_read_predicate_t(index::index_key_t&& index_key)
    : index_predicate_t(index_key)
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
    : index_predicate_t(index_key)
{
}

index_equal_range_predicate_t::index_equal_range_predicate_t(index::index_key_t&& index_key)
    : index_predicate_t(index_key)
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

const index::index_key_t& index_predicate_t::key() const
{
    return m_key;
}

} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
