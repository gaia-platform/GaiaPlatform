/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <type_cache.hpp>

#include <retail_assert.hpp>

using namespace std;
using namespace gaia::common;
using namespace gaia::db::types;

const reflection::Field* field_cache_t::get_field(uint16_t field_id) const
{
    field_map_t::const_iterator iterator = field_map.find(field_id);
    return (iterator == field_map.end()) ? nullptr : iterator->second;
}

void field_cache_t::set_field(uint16_t field_id, const reflection::Field* field)
{
    retail_assert(field != nullptr, "field_cache_t::set_field() should not be called with a null field value!");

    field_map.insert(make_pair(field_id, field));
}

type_cache_t* type_cache_t::get_type_cache()
{
    // If singleton instance has not been created,
    // acquire exclusive lock and create the instance,
    // unless another thread has already done it before us.
    if (s_type_cache == nullptr)
    {
        s_lock.lock();

        if (s_type_cache == nullptr)
        {
            s_type_cache = new type_cache_t();
        }

        s_lock.unlock();
    }

    return s_type_cache;
}

const field_cache_t* type_cache_t::get_field_cache(uint64_t type_id)
{
    // We keep a shared lock while the field_cache is in use,
    // to ensure that its information is not being updated by another thread.
    s_lock.lock_shared();

    type_map_t::const_iterator iterator = m_type_map.find(type_id);

    if (iterator == m_type_map.end())
    {
        s_lock.unlock_shared();

        return nullptr;
    }

    return iterator->second;
}

void type_cache_t::release_access()
{
    // Release the read lock taken by get_field_cache().
    // There is no check to ensure that a read lock was indeed being held by the caller of this method.
    s_lock.unlock_shared();
}

bool type_cache_t::remove_field_cache(uint64_t type_id)
{
    bool removed_field_cache = false;

    s_lock.lock();

    type_map_t::const_iterator iterator = m_type_map.find(type_id);
    if (iterator != m_type_map.end())
    {
        const field_cache_t* field_cache = iterator->second;
        m_type_map.erase(iterator);
        delete field_cache;
        removed_field_cache = true;
    }

    s_lock.unlock();

    return removed_field_cache;
}

bool type_cache_t::set_field_cache(uint64_t type_id, const field_cache_t* field_cache)
{
    retail_assert(field_cache != nullptr, "type_cache_t::set_field_cache() should not be called with a null cache!");

    bool inserted_field_cache = false;

    s_lock.lock();

    type_map_t::const_iterator iterator = m_type_map.find(type_id);
    if (iterator == m_type_map.end())
    {
        m_type_map.insert(make_pair(type_id, field_cache));
        inserted_field_cache = true;
    }

    s_lock.unlock();

    return inserted_field_cache;
}

auto_release_cache_read_access::auto_release_cache_read_access(bool enable)
{
    m_enabled = enable;
}

auto_release_cache_read_access::~auto_release_cache_read_access()
{
    if (m_enabled)
    {
        type_cache_t::get_type_cache()->release_access();
    }
}
