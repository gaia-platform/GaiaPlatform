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
enum access_lock_type
{
    none = 0,

    read = 1,
    insert = 2,
    update = 3,
    update_remove = 4,
    remove = 5,
};

struct access_control
{
    uint32_t readers_count;
    access_lock_type access_lock;

    access_control()
    {
        clear();
    }

    void clear()
    {
        readers_count = 0;
        access_lock = none;
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
class auto_access_control
{
    private:

    access_control* m_access_control;
    access_lock_type m_locked_access;
    bool m_has_marked_access;
    bool m_has_locked_access;

    public:
    
    auto_access_control();
    ~auto_access_control();

    void mark_access(access_control* pAccessControl);

    bool try_to_lock_access(access_control* pAccessControl, access_lock_type wantedAccess, access_lock_type& existingAccess);
    bool try_to_lock_access(access_control* pAccessControl, access_lock_type wantedAccess);

    // These versions can be called after an initial MarkAccess call.
    bool try_to_lock_access(access_lock_type wantedAccess, access_lock_type& existingAccess);
    bool try_to_lock_access(access_lock_type wantedAccess);

    // Methods for releasing all access or just the access lock.
    void release_access();
    void release_access_lock();

    bool has_marked_access()
    {
        return m_has_marked_access;
    }

    bool has_locked_access()
    {
        return m_has_locked_access;
    }

    private:

    void clear();
};

}
}
}

