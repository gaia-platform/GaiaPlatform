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
    m_pAccessControl = nullptr;
    m_lockedAccess = none;
    m_hasMarkedAccess = false;
    m_hasLockedAccess = false;
}

void CAutoAccessControl::mark_access(AccessControl* pAccessControl)
{
    retail_assert(pAccessControl != nullptr, "No access control was provided!");
    retail_assert(pAccessControl->readersCount != UINT32_MAX, "Readers count has maxed up and will overflow!");

    release_access();

    m_pAccessControl = pAccessControl;

    __sync_fetch_and_add(&m_pAccessControl->readersCount, 1);
    m_hasMarkedAccess = true;
}

bool CAutoAccessControl::try_to_lock_access(
    AccessControl* pAccessControl,
    EAccessLockType wantedAccess,
    EAccessLockType& existingAccess)
{
    retail_assert(wantedAccess != EAccessLockType::none, "Invalid wanted access!");

    mark_access(pAccessControl);

    m_lockedAccess = wantedAccess;
    existingAccess = __sync_val_compare_and_swap(&m_pAccessControl->accessLock, EAccessLockType::none, m_lockedAccess);
    m_hasLockedAccess = (existingAccess == EAccessLockType::none);

    return m_hasLockedAccess;
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
    retail_assert(m_pAccessControl != nullptr, "Invalid call, no access control available!");
    retail_assert(wantedAccess != EAccessLockType::none, "Invalid wanted access!");

    if (m_hasLockedAccess)
    {
        return m_lockedAccess == wantedAccess;
    }

    m_lockedAccess = wantedAccess;
    existingAccess = __sync_val_compare_and_swap(&m_pAccessControl->accessLock, EAccessLockType::none, m_lockedAccess);
    m_hasLockedAccess = (existingAccess == EAccessLockType::none);

    return m_hasLockedAccess;
}

bool CAutoAccessControl::try_to_lock_access(EAccessLockType wantedAccess)
{
    EAccessLockType existingAccess;

    return try_to_lock_access(wantedAccess, existingAccess);
}

void CAutoAccessControl::release_access()
{
    if (m_pAccessControl == nullptr)
    {
        return;
    }

    release_access_lock();

    if (m_hasMarkedAccess)
    {
        __sync_fetch_and_sub(&m_pAccessControl->readersCount, 1);
        m_hasMarkedAccess = false;
    }

    m_pAccessControl = nullptr;
}

void CAutoAccessControl::release_access_lock()
{
    if (m_pAccessControl == nullptr)
    {
        return;
    }

    if (m_hasLockedAccess)
    {
        retail_assert(
            __sync_bool_compare_and_swap(&m_pAccessControl->accessLock, m_lockedAccess, EAccessLockType::none),
            "Failed to release access lock!");
        m_hasLockedAccess = false;
    }

    m_lockedAccess = EAccessLockType::none;
}
