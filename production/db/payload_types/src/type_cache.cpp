/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_cache.hpp"

#include "retail_assert.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db::payload_types;

type_cache_t type_cache_t::s_type_cache;

const uint8_t* type_information_t::get_raw_binary_schema() const
{
    return m_binary_schema.data();
}

vector<uint8_t> type_information_t::get_serialization_template() const
{
    return m_serialization_template;
}

const reflection::Field* type_information_t::get_field(field_position_t field_position) const
{
    auto iterator = m_field_map.find(field_position);
    return (iterator == m_field_map.end()) ? nullptr : iterator->second;
}

void type_information_t::set_binary_schema(const vector<uint8_t>& binary_schema)
{
    m_binary_schema = binary_schema;
}

void type_information_t::set_serialization_template(const vector<uint8_t>& serialization_template)
{
    m_serialization_template = serialization_template;
}

void type_information_t::set_field(field_position_t field_position, const reflection::Field* field)
{
    retail_assert(field != nullptr, "type_information_t::set_field() should not be called with a null field value!");

    m_field_map.insert(make_pair(field_position, field));
}

size_t type_information_t::size()
{
    return m_field_map.size();
}

type_cache_t* type_cache_t::get()
{
    return &s_type_cache;
}

void type_cache_t::get_type_information(gaia_type_t type_id, auto_type_information_t& auto_type_information) const
{
    // We keep a shared lock while the type_information is in use,
    // to ensure that its information is not being updated by another thread.
    m_lock.lock_shared();

    auto iterator = m_type_map.find(type_id);

    if (iterator == m_type_map.end())
    {
        m_lock.unlock_shared();
    }
    else
    {
        auto_type_information.set(iterator->second);
    }
}

bool type_cache_t::remove_type_information(gaia_type_t type_id)
{
    bool removed_type_information = false;

    unique_lock unique_lock(m_lock);

    auto iterator = m_type_map.find(type_id);
    if (iterator != m_type_map.end())
    {
        const type_information_t* type_information = iterator->second;
        m_type_map.erase(iterator);
        delete type_information;
        removed_type_information = true;
    }

    return removed_type_information;
}

bool type_cache_t::set_type_information(gaia_type_t type_id, const type_information_t* type_information)
{
    retail_assert(type_information != nullptr, "type_cache_t::set_type_information() should not be called with a null cache!");

    bool inserted_type_information = false;

    unique_lock unique_lock(m_lock);

    auto iterator = m_type_map.find(type_id);
    if (iterator == m_type_map.end())
    {
        m_type_map.insert(make_pair(type_id, type_information));
        inserted_type_information = true;
    }

    return inserted_type_information;
}

size_t type_cache_t::size() const
{
    return m_type_map.size();
}

auto_type_information_t::auto_type_information_t()
{
    m_type_information = nullptr;
}

auto_type_information_t::~auto_type_information_t()
{
    if (m_type_information != nullptr)
    {
        type_cache_t::get()->m_lock.unlock_shared();
    }
}

const type_information_t* auto_type_information_t::get()
{
    return m_type_information;
}

void auto_type_information_t::set(const type_information_t* type_information)
{
    retail_assert(
        m_type_information == nullptr,
        "auto_type_information_t::set() was called on an already set instance!");

    m_type_information = type_information;
}
