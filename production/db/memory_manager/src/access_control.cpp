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

CAutoAccessControl::CAutoAccessControl()
{
    clear();
}

CAutoAccessControl::~CAutoAccessControl()
{
    release_access();
}

void CAutoAccessControl::clear()
{
    m_access_control = nullptr;
    m_locked_access = none;
    m_has_marked_access = false;
    m_has_locked_access = false;
}

void CAutoAccessControl::mark_access(AccessControl* pAccessControl)
{
    retail_assert(pAccessControl != nullptr, "No access control was provided!");
    retail_assert(pAccessControl->readers_count != UINT32_MAX, "Readers count has maxed up and will overflow!");

    release_access();

    m_access_control = pAccessControl;

    __sync_fetch_and_add(&m_access_control->readers_count, 1);
    m_has_marked_access = true;
}

bool CAutoAccessControl::try_to_lock_access(
    AccessControl* pAccessControl,
    EAccessLockType wantedAccess,
    EAccessLockType& existingAccess)
{
    retail_assert(wantedAccess != EAccessLockType::none, "Invalid wanted access!");

    mark_access(pAccessControl);

    m_locked_access = wantedAccess;
    existingAccess = __sync_val_compare_and_swap(&m_access_control->access_lock, EAccessLockType::none, m_locked_access);
    m_has_locked_access = (existingAccess == EAccessLockType::none);

    return m_has_locked_access;
}

bool CAutoAccessControl::try_to_lock_access(AccessControl* pAccessControl, EAccessLockType wantedAccess)
{
    EAccessLockType existingAccess;

    return try_to_lock_access(pAccessControl, wantedAccess, existingAccess);
}

bool CAutoAccessControl::try_to_lock_access(
    EAccessLockType wantedAccess,
    EAccessLockType& existingAccess)
{
    retail_assert(m_access_control != nullptr, "Invalid call, no access control available!");
    retail_assert(wantedAccess != EAccessLockType::none, "Invalid wanted access!");

    if (m_has_locked_access)
    {
        return m_locked_access == wantedAccess;
    }

    m_locked_access = wantedAccess;
    existingAccess = __sync_val_compare_and_swap(&m_access_control->access_lock, EAccessLockType::none, m_locked_access);
    m_has_locked_access = (existingAccess == EAccessLockType::none);

    return m_has_locked_access;
}

bool CAutoAccessControl::try_to_lock_access(EAccessLockType wantedAccess)
{
    EAccessLockType existingAccess;

    return try_to_lock_access(wantedAccess, existingAccess);
}

void CAutoAccessControl::release_access()
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

void CAutoAccessControl::release_access_lock()
{
    if (m_access_control == nullptr)
    {
        return;
    }

    if (m_has_locked_access)
    {
        retail_assert(
            __sync_bool_compare_and_swap(&m_access_control->access_lock, m_locked_access, EAccessLockType::none),
            "Failed to release access lock!");
        m_has_locked_access = false;
    }

    m_locked_access = EAccessLockType::none;
}
