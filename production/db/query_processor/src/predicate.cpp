/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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

bool index_predicate_t::filter(const gaia_ptr_t&) const
{
    return true;
}

gaia::db::messages::index_query_t index_predicate_t::query_type() const
{
    return messages::index_query_t::NONE;
}

serialized_index_query_t index_predicate_t::as_query(flatbuffers::FlatBufferBuilder&) const
{
    return c_null_predicate;
}

index_point_read_predicate_t::index_point_read_predicate_t(index::index_key_t index_key)
    : m_key(std::move(index_key))
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

serialized_index_query_t index_point_read_predicate_t::as_query(flatbuffers::FlatBufferBuilder& fbb) const
{
    data_write_buffer_t buf(fbb);
    index::index_builder_t::serialize_key(m_key, buf);
    auto output = buf.output();

    auto query = messages::Createindex_point_read_query_t(fbb, output);

    return query.Union();
}

index_equal_range_predicate_t::index_equal_range_predicate_t(index::index_key_t index_key)
    : m_key(std::move(index_key))
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

serialized_index_query_t index_equal_range_predicate_t::as_query(flatbuffers::FlatBufferBuilder& fbb) const
{
    data_write_buffer_t buf(fbb);
    index::index_builder_t::serialize_key(m_key, buf);
    auto output = buf.output();

    auto query = messages::Createindex_equal_range_query_t(fbb, output);

    return query.Union();
}

} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
