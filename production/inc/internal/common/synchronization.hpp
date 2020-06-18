/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <pthread.h>

#include <exceptions.hpp>

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup Common
 * @{
 */

/**
 * The exception class used for unexpected pwthread_rwlock errors.
 */
class pthread_rwlock_error : public api_error
{
public:
    pthread_rwlock_error(const char* api_name, int error_code)
        : api_error(api_name, error_code)
    {
    }
};

/**
 * An implementation of the C17 shared_mutex interfaces using pthread_rwlock.
 *
 * The main goal of this implementation is to facilitate a future transition
 * to the standard shared_mutex implementation.
 */
class shared_mutex
{
public:

    shared_mutex();
    ~shared_mutex();

    void lock();
    bool try_lock();
    void unlock();

    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();

protected:

    pthread_rwlock_t m_lock;
};

/**
 * A class for automatically acquiring and releasing a shared_mutex lock.
 */
class auto_lock
{
public:

    auto_lock(shared_mutex& lock, bool request_shared = false);
    ~auto_lock();

protected:

    shared_mutex* m_lock;
    bool m_request_shared;
};



/*@}*/
}
/*@}*/
}
