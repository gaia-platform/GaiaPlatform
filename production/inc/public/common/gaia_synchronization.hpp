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


class pthread_rwlock_error: public gaia::common::gaia_exception
{
public:
    pthread_rwlock_error(const char* api_name, int error_code);
};

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
