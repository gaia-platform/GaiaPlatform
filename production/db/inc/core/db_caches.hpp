////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <unordered_map>
#include <unordered_set>

#include "gaia/common.hpp"

namespace gaia
{
namespace db
{
namespace caches
{

// These caches are meant to be read-only.
// They should be initialized safely during client or server setup,
// so they require no special synchronization to access.
//
// These caches are meant to be used exclusively with non-DDL sessions,
// when metadata information is immutable (and thus can be safely cached).
//
// Check whether a cache has been initialized before using it!

typedef std::unordered_set<common::field_position_t> field_position_set_t;

typedef std::unordered_map<common::gaia_id_t, std::pair<field_position_set_t, field_position_set_t>>
    table_relationship_fields_map_t;

// The base db cache class provides functionality common to all caches.
class base_db_cache_t
{
protected:
    // Indicates whether the cache was initialized.
    // The caches will not be initialized for DDL sessions,
    // to prevent their information from becoming outdated as a result of DDL operations.
    bool m_is_initialized{false};
};

// A cache that collects parent and child relationship fields that can be auto-connected.
// For each table, the cache stores a pair of sets that represent the parent and child field positions.
// The first element of the pair is for the parent relationships and the second is for the child relationships .
class table_relationship_fields_cache_t : public base_db_cache_t
{
protected:
    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    table_relationship_fields_cache_t(const table_relationship_fields_cache_t&) = delete;
    table_relationship_fields_cache_t& operator=(const table_relationship_fields_cache_t&) = delete;

public:
    table_relationship_fields_cache_t() = default;

    bool is_initialized();
    void clear();

    // Each table gets an entry, regardless of whether it is involved in a relationship or not.
    // This eliminates having to check for special situations during lookups.
    void put(common::gaia_id_t table_id);
    void put_parent_relationship_field(common::gaia_id_t table_id, common::field_position_t field);
    void put_child_relationship_field(common::gaia_id_t table_id, common::field_position_t field);
    const std::pair<field_position_set_t, field_position_set_t>& get(common::gaia_id_t table_id) const;

    size_t size() const;

protected:
    // The map used by the cache.
    table_relationship_fields_map_t m_map;
};

// This structure collects all db_caches, to avoid having to declare them individually elsewhere.
struct db_caches_t
{
    table_relationship_fields_cache_t table_relationship_fields_cache;
};

} // namespace caches
} // namespace db
} // namespace gaia
