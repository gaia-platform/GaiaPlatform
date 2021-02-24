/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_cache.hpp"

#include "gaia_internal/common/retail_assert.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db::payload_types;

type_cache_t type_cache_t::s_type_cache;

void type_information_t::set_binary_schema(const uint8_t* binary_schema, size_t binary_schema_size)
{
    m_binary_schema = binary_schema;
    m_binary_schema_size = binary_schema_size;
}

void type_information_t::set_serialization_template(
    const uint8_t* serialization_template,
    size_t serialization_template_size)
{
    m_serialization_template = serialization_template;
    m_serialization_template_size = serialization_template_size;
}

void type_information_t::set_field(field_position_t field_position, const reflection::Field* field)
{
    retail_assert(field != nullptr, "type_information_t::set_field() should not be called with a null field value!");

    m_field_map.insert(make_pair(field_position, field));
}

const uint8_t* type_information_t::get_raw_binary_schema() const
{
    return m_binary_schema;
}

size_t type_information_t::get_binary_schema_size() const
{
    return m_binary_schema_size;
}

const uint8_t* type_information_t::get_serialization_template() const
{
    return m_serialization_template;
}

size_t type_information_t::get_serialization_template_size() const
{
    return m_serialization_template_size;
}

const reflection::Field* type_information_t::get_field(field_position_t field_position) const
{
    auto iterator = m_field_map.find(field_position);
    return (iterator == m_field_map.end()) ? nullptr : iterator->second;
}

size_t type_information_t::get_field_count() const
{
    return m_field_map.size();
}

type_cache_t* type_cache_t::get()
{
    return &s_type_cache;
}

bool type_cache_t::get_type_information(gaia_type_t type_id, auto_type_information_t& auto_type_information) const
{
    // We acquire a shared lock while we retrieve the type information,
    // to ensure that the cache is not being updated by another thread.
    shared_lock shared_lock(m_lock);

    auto iterator = m_type_map.find(type_id);

    if (iterator == m_type_map.end())
    {
        return false;
    }

    // We hold a shared lock on the type information while it is used,
    // to ensure that it is not removed from the cache.
    iterator->second->m_lock.lock_shared();

    // To ensure that the lock is released eventually,
    // we package the type information in an auto object.
    auto_type_information.set(iterator->second);

    return true;
}

bool type_cache_t::remove_type_information(gaia_type_t type_id)
{
    bool removed_type_information = false;

    unique_lock unique_lock(m_lock);

    auto iterator = m_type_map.find(type_id);
    if (iterator != m_type_map.end())
    {
        // Take an exclusive lock on the record that we want to remove.
        // This will ensure that nobody else is reading it.
        iterator->second->m_lock.lock();

        m_type_map.erase(iterator);
        removed_type_information = true;
    }

    return removed_type_information;
}

bool type_cache_t::set_type_information(gaia_type_t type_id, unique_ptr<type_information_t>& type_information)
{
    retail_assert(
        !!type_information,
        "type_cache_t::set_type_information() should not be called with a null cache!");

    bool inserted_type_information = false;

    unique_lock unique_lock(m_lock);

    auto iterator = m_type_map.find(type_id);
    if (iterator == m_type_map.end())
    {
        // Transfer unique_ptr information into a shared_ptr const object.
        shared_ptr<const type_information_t> const_type_information;
        const_type_information.reset(type_information.release());

        m_type_map.insert(make_pair(type_id, const_type_information));
        inserted_type_information = true;
    }

    return inserted_type_information;
}

size_t type_cache_t::size() const
{
    return m_type_map.size();
}

auto_type_information_t::~auto_type_information_t()
{
    if (m_type_information)
    {
        m_type_information->m_lock.unlock_shared();
    }
}

const type_information_t* auto_type_information_t::get()
{
    return m_type_information ? m_type_information.get() : nullptr;
}

void auto_type_information_t::set(const shared_ptr<const type_information_t>& type_information)
{
    retail_assert(
        !m_type_information,
        "auto_type_information_t::set() was called on an already set instance!");

    m_type_information = type_information;
}
