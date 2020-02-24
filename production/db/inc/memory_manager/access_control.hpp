/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia
{
namespace db
{
namespace memory_manager
{

// Values that can indicate the access desired for resources.
enum EAccessLockType
{
    alt_None = 0,

    alt_Read = 1,
    alt_Insert = 2,
    alt_Update = 3,
    alt_UpdateRemove = 4,
    alt_Remove = 5,
};

struct AccessControl
{
    uint32_t readersCount;
    EAccessLockType accessLock;

    AccessControl()
    {
        Clear();
    }

    void Clear()
    {
        readersCount = 0;
        accessLock = alt_None;
    }
};

// A class for access synchronization. It can be used to implement spinlocks.
//
// This class manipulates the access control object that is associated to a resource.
//
// There are 2 main operations:
// (i) Marking read access - this is a ref-count that can be used to prevent the
// destruction of a resource until all readers have finished accessing it.
// But note that because a resource can be changed between when its access control
// was first read and when it was updated, some additional checks need to be done
// after the read access was marked.
// (ii) Locking access - this is an exclusive lock that prevents other threads
// from operating on the resource.
//
class CAutoAccessControl
{
    private:

    AccessControl* m_pAccessControl;
    EAccessLockType m_lockedAccess;
    bool m_hasMarkedAccess;
    bool m_hasLockedAccess;

    public:
    
    CAutoAccessControl();
    ~CAutoAccessControl();

    void MarkAccess(AccessControl* pAccessControl);

    bool TryToLockAccess(AccessControl* pAccessControl, EAccessLockType wantedAccess, EAccessLockType& existingAccess);
    bool TryToLockAccess(AccessControl* pAccessControl, EAccessLockType wantedAccess);

    // These versions can be called after an initial MarkAccess call.
    bool TryToLockAccess(EAccessLockType wantedAccess, EAccessLockType& existingAccess);
    bool TryToLockAccess(EAccessLockType wantedAccess);

    // Methods for releasing all access or just the access lock.
    void ReleaseAccess();
    void ReleaseAccessLock();

    bool HasMarkedAccess()
    {
        return m_hasMarkedAccess;
    }

    bool HasLockedAccess()
    {
        return m_hasLockedAccess;
    }

    private:

    void Clear();
};

}
}
}

