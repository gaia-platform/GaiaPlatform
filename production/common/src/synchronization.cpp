/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <synchronization.hpp>

#include <sstream>
#include <string>

#include <gaia_exception.hpp>
#include <retail_assert.hpp>

using namespace std;
using namespace gaia::common;

pthread_rwlock_error::pthread_rwlock_error(const char* api_name, int error_code)
{
    stringstream string_stream;
    string_stream << api_name << "() returned an unexpected error code: " << error_code << ".";
    m_message = string_stream.str();
}

shared_mutex::shared_mutex()
{
    int error_code = pthread_rwlock_init(&m_lock, nullptr);
    if (error_code != 0)
    {
        throw pthread_rwlock_error("pthread_rwlock_init", error_code);
    }
}

shared_mutex::~shared_mutex()
{
    // We can't throw exceptions and we don't have another way of surfacing errors.
    // OTOH, it's unlikely that such mutexes would get destroyed during regular code execution anyway,
    // as they'd normally be used to protect access to long-lived resources.
    pthread_rwlock_destroy(&m_lock);
}

void shared_mutex::lock()
{
    int error_code = pthread_rwlock_wrlock(&m_lock);
    if (error_code != 0)
    {
        throw pthread_rwlock_error("pthread_rwlock_wrlock", error_code);
    }
}

bool shared_mutex::try_lock()
{
    int error_code = pthread_rwlock_trywrlock(&m_lock);
    if (error_code == 0)
    {
        return true;
    }
    else if (error_code == EBUSY)
    {
        return false;
    }
    else
    {
        throw pthread_rwlock_error("pthread_rwlock_trywrlock", error_code);
    }
}

void shared_mutex::unlock()
{
    int error_code = pthread_rwlock_unlock(&m_lock);
    if (error_code != 0)
    {
        throw pthread_rwlock_error("pthread_rwlock_unlock", error_code);
    }
}

void shared_mutex::lock_shared()
{
    int error_code = pthread_rwlock_rdlock(&m_lock);
    if (error_code != 0)
    {
        throw pthread_rwlock_error("pthread_rwlock_rdlock", error_code);
    }
}

bool shared_mutex::try_lock_shared()
{
    int error_code = pthread_rwlock_tryrdlock(&m_lock);
    if (error_code == 0)
    {
        return true;
    }
    else if (error_code == EBUSY)
    {
        return false;
    }
    else
    {
        throw pthread_rwlock_error("pthread_rwlock_tryrdlock", error_code);
    }
}

void shared_mutex::unlock_shared()
{
    unlock();
}
