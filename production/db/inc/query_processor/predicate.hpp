/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/db/gaia_ptr.hpp"

#include "index.hpp"
#include "messages_generated.h"

namespace gaia
{
namespace db
{
namespace query_processor
{
namespace scan
{

using serialized_index_query_t = flatbuffers::Offset<void>;

constexpr int c_null_predicate = 0;

// Default index predicate results in a full scan.
class index_predicate_t
{
public:
    index_predicate_t() = default;
    virtual ~index_predicate_t() = default;

    // This is the client-side filter.
    // It does additional filtering on the client on top of the index query on the server.
    // By default, no additional filtering is done.
    virtual bool filter(const gaia_ptr_t& ptr) const;

    // Type of index query to push down to server.
    // By default, NONE.
    virtual gaia::db::messages::index_query_t query_type() const;

    // flatbuffers query object.
    // By default, returns empty flatbuffers union.
    virtual serialized_index_query_t as_query(flatbuffers::FlatBufferBuilder& fbb) const;
};

// Point read predicate for indexes.
class index_point_read_predicate_t : public index_predicate_t
{
public:
    explicit index_point_read_predicate_t(index::index_key_t index_key);
    explicit index_point_read_predicate_t(index::index_key_t&& index_key);
    ~index_point_read_predicate_t() override = default;

    gaia::db::messages::index_query_t query_type() const override;
    serialized_index_query_t as_query(flatbuffers::FlatBufferBuilder& fbb) const override;

private:
    index::index_key_t m_key;
};

// Equal range predicate for indexes.
class index_equal_range_predicate_t : public index_predicate_t
{
public:
    explicit index_equal_range_predicate_t(index::index_key_t index_key);
    explicit index_equal_range_predicate_t(index::index_key_t&& index_key);
    ~index_equal_range_predicate_t() override = default;

    gaia::db::messages::index_query_t query_type() const override;
    serialized_index_query_t as_query(flatbuffers::FlatBufferBuilder& fbb) const override;

private:
    index::index_key_t m_key;
};

} // namespace scan
} // namespace query_processor
} // namespace db
}; // namespace gaia
