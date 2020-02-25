/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>

#include "retail_assert.hpp"
#include "access_control.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;

auto_access_control_t::auto_access_control_t()
{
    clear();
}

auto_access_control_t::~auto_access_control_t()
{
    release_access();
}

void auto_access_control_t::clear()
{
    m_access_control = nullptr;
    m_locked_access = none;
    m_has_marked_access = false;
    m_has_locked_access = false;
}

void auto_access_control_t::mark_access(access_control_t* access_control)
{
    retail_assert(access_control != nullptr, "No access control was provided!");
    retail_assert(access_control->readers_count != UINT32_MAX, "Readers count has maxed up and will overflow!");

    release_access();

    m_access_control = access_control;

    __sync_fetch_and_add(&m_access_control->readers_count, 1);
    m_has_marked_access = true;
}

bool auto_access_control_t::try_to_lock_access(
    access_control_t* access_control,
    access_lock_type_t wanted_access,
    access_lock_type_t& existing_access)
{
    retail_assert(wanted_access != access_lock_type_t::none, "Invalid wanted access!");

    mark_access(access_control);

    m_locked_access = wanted_access;
    existing_access = __sync_val_compare_and_swap(&m_access_control->access_lock, access_lock_type_t::none, m_locked_access);
    m_has_locked_access = (existing_access == access_lock_type_t::none);

    return m_has_locked_access;
}

bool auto_access_control_t::try_to_lock_access(access_control_t* access_control, access_lock_type_t wanted_access)
{
    access_lock_type_t existing_access;

    return try_to_lock_access(access_control, wanted_access, existing_access);
}

bool auto_access_control_t::try_to_lock_access(
    access_lock_type_t wanted_access,
    access_lock_type_t& existing_access)
{
    retail_assert(m_access_control != nullptr, "Invalid call, no access control available!");
    retail_assert(wanted_access != access_lock_type_t::none, "Invalid wanted access!");

    if (m_has_locked_access)
    {
        return m_locked_access == wanted_access;
    }

    m_locked_access = wanted_access;
    existing_access = __sync_val_compare_and_swap(&m_access_control->access_lock, access_lock_type_t::none, m_locked_access);
    m_has_locked_access = (existing_access == access_lock_type_t::none);

    return m_has_locked_access;
}

bool auto_access_control_t::try_to_lock_access(access_lock_type_t wanted_access)
{
    access_lock_type_t existing_access;

    return try_to_lock_access(wanted_access, existing_access);
}

void auto_access_control_t::release_access()
{
    if (m_access_control == nullptr)
    {
        return;
    }

    release_access_lock();

    if (m_has_marked_access)
    {
        __sync_fetch_and_sub(&m_access_control->readers_count, 1);
        m_has_marked_access = false;
    }

    m_access_control = nullptr;
}

void auto_access_control_t::release_access_lock()
{
    if (m_access_control == nullptr)
    {
        return;
    }

    if (m_has_locked_access)
    {
        retail_assert(
            __sync_bool_compare_and_swap(&m_access_control->access_lock, m_locked_access, access_lock_type_t::none),
            "Failed to release access lock!");
        m_has_locked_access = false;
    }

    m_locked_access = access_lock_type_t::none;
}
