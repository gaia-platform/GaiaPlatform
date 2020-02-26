/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stdint.h>

namespace gaia
{
namespace db
{
namespace memory_manager
{

// Values that can indicate the access desired for resources.
enum class access_lock_type_t
{
    none = 0,

    read = 1,
    insert = 2,
    update = 3,
    update_remove = 4,
    remove = 5,
};

struct access_control_t
{
    uint32_t readers_count;
    access_lock_type_t access_lock;

    access_control_t()
    {
        clear();
    }

    void clear()
    {
        readers_count = 0;
        access_lock = access_lock_type_t::none;
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
class auto_access_control_t
{
    private:

    access_control_t* m_access_control;
    access_lock_type_t m_locked_access;
    bool m_has_marked_access;
    bool m_has_locked_access;

    public:
    
    auto_access_control_t();
    ~auto_access_control_t();

    void mark_access(access_control_t* access_control);

    bool try_to_lock_access(access_control_t* access_control, access_lock_type_t wanted_access, access_lock_type_t& existing_access);
    bool try_to_lock_access(access_control_t* access_control, access_lock_type_t wanted_access);

    // These versions can be called after an initial MarkAccess call.
    bool try_to_lock_access(access_lock_type_t wanted_access, access_lock_type_t& existing_access);
    bool try_to_lock_access(access_lock_type_t wanted_access);

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
