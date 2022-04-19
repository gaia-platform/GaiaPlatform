/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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

typedef std::unordered_map<common::gaia_id_t, field_position_set_t> table_relationship_field_map_t;

class table_relationship_cache_t
{
protected:
    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    table_relationship_cache_t(const table_relationship_cache_t&) = delete;
    table_relationship_cache_t& operator=(const table_relationship_cache_t&) = delete;

    // table_relationship_cache_t is a singleton, so its constructor is not public.
    table_relationship_cache_t() = default;

public:
    // Return a pointer to the singleton instance.
    static table_relationship_cache_t* get();

    bool is_initialized();

    void put(common::gaia_id_t table_id);
    void put(common::gaia_id_t table_id, common::field_position_t field);
    field_position_set_t& get(common::gaia_id_t table_id);

    size_t size() const;

protected:
    // The singleton instance.
    static table_relationship_cache_t s_cache;

    // The map used by the cache.
    table_relationship_field_map_t m_map;

    // Indicates whether the cache was initialized.
    bool m_is_initialized{false};
};

} // namespace caches
} // namespace db
} // namespace gaia
