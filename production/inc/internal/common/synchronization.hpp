/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <pthread.h>

#include <gaia_exception.hpp>

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
class pthread_rwlock_error : public gaia::common::gaia_exception
{
public:
    pthread_rwlock_error(const char* api_name, int error_code);
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

/*@}*/
}
/*@}*/
}
