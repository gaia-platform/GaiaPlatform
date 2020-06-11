/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <unordered_map>

#include "flatbuffers/reflection.h"

#include <gaia_synchronization.hpp>

namespace gaia
{
namespace db
{
namespace types
{

typedef std::unordered_map<uint16_t, const reflection::Field*> field_map_t;

struct field_cache_t
{
    static void release(field_cache_t* field_cache);

    field_cache_t();

    const reflection::Field* get_field(uint16_t field_id) const;

    field_map_t field_map;

    // We use this reference count to control deallocation of field cache memory.
    uint64_t reference_count;
};

typedef std::unordered_map<uint64_t, field_cache_t*> type_map_t;

class type_cache_t
{
private:

    // type_cache_t is a singleton, so its constructor is private.
    type_cache_t() = default;

public:

    // Returns a pointer to the singleton instance.
    static type_cache_t* get_type_cache();

    // Caller should call release() on the field cache, once he is done using it.
    const field_cache_t* get_field_cache(uint64_t type_id);

    void clear_field_cache(uint64_t type_id);

    void set_field_cache(uint64_t type_id, field_cache_t* field_cache);

protected:

    // Operations that require exclusive locking are meant to be rare.
    // We can further improve implementation by preloading type information on system startup.
    static gaia::common::shared_mutex s_lock;

    // The singleton instance, created on first request.
    static type_cache_t* s_type_cache;

    type_map_t m_type_map;
};

class auto_release_field_cache
{
public:

    auto_release_field_cache(const field_cache_t* field_cache);
    auto_release_field_cache(field_cache_t* field_cache);
    ~auto_release_field_cache();

protected:

    field_cache_t* m_field_cache;
};

}
}
}
