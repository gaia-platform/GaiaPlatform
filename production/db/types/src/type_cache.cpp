/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <type_cache.hpp>

#include <retail_assert.hpp>

using namespace std;
using namespace gaia::common;
using namespace gaia::db::types;

void field_cache_t::release(field_cache_t* field_cache)
{
    retail_assert(field_cache != nullptr, "field_cache_t::release() should not be called with a null cache!");

    __sync_fetch_and_sub(&(field_cache->reference_count), 1);
    if (field_cache->reference_count == 0)
    {
        delete field_cache;
    }
}

field_cache_t::field_cache_t()
{
    reference_count = 1;
}

const reflection::Field* field_cache_t::get_field(uint16_t field_id) const
{
    field_map_t::const_iterator iterator = field_map.find(field_id);
    return (iterator == field_map.end()) ? nullptr : iterator->second;
}

type_cache_t* type_cache_t::get_type_cache()
{
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
    s_lock.lock_shared();

    type_map_t::const_iterator iterator = m_type_map.find(type_id);

    if (iterator == m_type_map.end())
    {
        return nullptr;
    }
    else
    {
        field_cache_t* field_cache = iterator->second;
        __sync_fetch_and_add(&(field_cache->reference_count), 1);
        return field_cache;
    }

    s_lock.unlock_shared();
}

void type_cache_t::clear_field_cache(uint64_t type_id)
{
    s_lock.lock();

    type_map_t::const_iterator iterator = m_type_map.find(type_id);
    if (iterator != m_type_map.end())
    {
        field_cache_t* field_cache = iterator->second;
        __sync_fetch_and_sub(&(field_cache->reference_count), 1);
        m_type_map.erase(iterator);
    }

    s_lock.unlock();
}

void type_cache_t::set_field_cache(uint64_t type_id, field_cache_t* field_cache)
{
    retail_assert(field_cache != nullptr, "type_cache_t::set_field_cache() should not be called with a null cache!");

    s_lock.lock();

    m_type_map.erase(type_id);
    m_type_map.insert(make_pair(type_id, field_cache));

    s_lock.unlock();
}

auto_release_field_cache::auto_release_field_cache(const field_cache_t* field_cache)
{
    m_field_cache = const_cast<field_cache_t*>(field_cache);
}

auto_release_field_cache::auto_release_field_cache(field_cache_t* field_cache)
{
    m_field_cache = field_cache;
}

auto_release_field_cache::~auto_release_field_cache()
{
    if (m_field_cache != nullptr)
    {
        field_cache_t::release(m_field_cache);
    }
}
