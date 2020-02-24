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
    none = 0,

    read = 1,
    insert = 2,
    update = 3,
    update_remove = 4,
    remove = 5,
};

struct AccessControl
{
    uint32_t readersCount;
    EAccessLockType accessLock;

    AccessControl()
    {
        clear();
    }

    void clear()
    {
        readersCount = 0;
        accessLock = none;
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

    void mark_access(AccessControl* pAccessControl);

    bool try_to_lock_access(AccessControl* pAccessControl, EAccessLockType wantedAccess, EAccessLockType& existingAccess);
    bool try_to_lock_access(AccessControl* pAccessControl, EAccessLockType wantedAccess);

    // These versions can be called after an initial MarkAccess call.
    bool try_to_lock_access(EAccessLockType wantedAccess, EAccessLockType& existingAccess);
    bool try_to_lock_access(EAccessLockType wantedAccess);

    // Methods for releasing all access or just the access lock.
    void release_access();
    void release_access_lock();

    bool has_marked_access()
    {
        return m_hasMarkedAccess;
    }

    bool has_locked_access()
    {
        return m_hasLockedAccess;
    }

    private:

    void clear();
};

}
}
}

