/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <shared_mutex>
#include <unordered_map>

#include "flatbuffers/reflection.h"
#include "gaia_common.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace payload_types
{

typedef std::unordered_map<field_position_t, const reflection::Field*> field_map_t;

// A field cache stores all Field descriptions for a given type,
// indexed by the corresponding field id values.
class field_cache_t
{
public:

    field_cache_t() = default;

    // Return field information if the field could be found or nullptr otherwise.
    //
    // Because the field information is retrieved by direct access to the binary schema,
    // we need to maintain a read lock while it is being used,
    // to prevent it from being changed.
    const reflection::Field* get_field(field_position_t field_position) const;

    // Insert information about a field in the cache.
    // This is used during construction of the cache.
    void set_field(field_position_t field_position, const reflection::Field* field);

    // Return the size of the internal map.
    size_t size();

protected:

    // The map used by the field cache.
    field_map_t m_field_map;
};

typedef std::unordered_map<gaia_type_t, const field_cache_t*> type_map_t;

class auto_field_cache_t;

// The type cache stores field_caches for all managed types.
// The field_caches are indexed by their corresponding type id.
class type_cache_t
{
    friend class auto_field_cache_t;

protected:

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    type_cache_t(const type_cache_t&) = delete;
    type_cache_t& operator=(const type_cache_t&) = delete;

    // type_cache_t is a singleton, so its constructor is not public.
    type_cache_t() = default;

public:

    // Return a pointer to the singleton instance.
    static type_cache_t* get();

    // To ensure that the returned field cache continues to be valid,
    // this method needs to hold a read lock on the type cache.
    // To ensure the release of that lock once the field cache is no longer used,
    // it is returned in an auto_field_cache_t wrapper that will release the lock
    // at the time the wrapper gets destroyed.
    void get_field_cache(gaia_type_t type_id, auto_field_cache_t& auto_field_cache) const;

    // This method should be called whenever the information for a type is being changed.
    // It will return true if the entry was found and deleted, and false if it was not found
    // (another thread may have deleted it first or the information may never have been cached at all).
    bool remove_field_cache(gaia_type_t type_id);

    // This method should be used to load new type information in the cache.
    // It expects the cache to contain no data for the type.
    // It returns true if the cache was updated and false if an entry for the type was found to exist already.
    bool set_field_cache(gaia_type_t type_id, const field_cache_t* field_cache);

    // Return the size of the internal map.
    size_t size() const;

protected:

    // The singleton instance.
    static type_cache_t s_type_cache;

    // Reads from cache will hold read locks, whereas update operations will request exclusive locks.
    // Operations that require exclusive locking are meant to be rare.
    // We can further improve implementation by preloading type information at system startup.
    mutable std::shared_mutex m_lock;

    // The map used by the type cache.
    type_map_t m_type_map;
};

// A class for automatically releasing the read lock taken while reading from the cache.
class auto_field_cache_t
{
    friend class type_cache_t;

public:

    auto_field_cache_t();
    ~auto_field_cache_t();

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    auto_field_cache_t(const auto_field_cache_t&) = delete;
    auto_field_cache_t& operator=(const auto_field_cache_t&) = delete;

    const field_cache_t* get();

protected:

    const field_cache_t* m_field_cache;

    void set(const field_cache_t* field_cache);
};

}
}
}
