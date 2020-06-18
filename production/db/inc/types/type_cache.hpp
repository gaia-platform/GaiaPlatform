/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <unordered_map>

#include "flatbuffers/reflection.h"

#include <synchronization.hpp>

namespace gaia
{
namespace db
{
namespace types
{

typedef std::unordered_map<uint16_t, const reflection::Field*> field_map_t;

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
    const reflection::Field* get_field(uint16_t field_id) const;

    // Insert information about a field in the cache.
    // This is used during construction of the cache.
    void set_field(uint16_t field_id, const reflection::Field* field);

    // Return the size of the internal map.
    size_t size();

protected:

    // The map used by the field cache.
    field_map_t m_field_map;
};

typedef std::unordered_map<uint64_t, const field_cache_t*> type_map_t;

// The type cache stores field_caches for all managed types.
// The field_caches are indexed by their corresponding type id.
class type_cache_t
{
protected:

    // type_cache_t is a singleton, so its constructor is not public.
    type_cache_t() = default;

public:

    // Return a pointer to the singleton instance.
    static type_cache_t* get_type_cache();

    // Caller should call release_access() to release the read lock taken by get_field_cache().
    // This is not necessary if get_field_cache() returned nullptr.
    // Alternatively, auto_release_cache_read_access can be used to automatically release the access.
    const field_cache_t* get_field_cache(uint64_t type_id);
    void release_access();

    // This method should be called whenever the information for a type is being changed.
    // It will return true if the entry was found and deleted, and false if it was not found
    // (another thread may have deleted it first or the information may never have been cached at all).
    bool remove_field_cache(uint64_t type_id);

    // This method should be used to load new type information in the cache.
    // It expects the cache to contain no data for the type.
    // It returns true if the cache was updated and false if an entry for the type was found to exist already.
    bool set_field_cache(uint64_t type_id, const field_cache_t* field_cache);

    // Return the size of the internal map.
    size_t size();

protected:

    // The singleton instance.
    static type_cache_t s_type_cache;

    // Reads from cache will hold read locks, whereas update operations will request exclusive locks.
    // Operations that require exclusive locking are meant to be rare.
    // We can further improve implementation by preloading type information at system startup.
    gaia::common::shared_mutex m_lock;

    // The map used by the type cache.
    type_map_t m_type_map;
};

// A class for automatically releasing the read lock taken while reading from the cache.
class auto_release_cache_read_access
{
public:

    auto_release_cache_read_access(bool enable);
    ~auto_release_cache_read_access();

protected:

    bool m_enable_release;
};

}
}
}
