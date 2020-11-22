/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stdexcept>

#include <libexplain/mmap.h>
#include <libexplain/munmap.h>
#include <sys/mman.h>

#include "gaia/gaia_exception.hpp"
#include "system_error.hpp"

namespace gaia
{
namespace common
{

inline void* map_fd(size_t length, int protection, int flags, int fd, size_t offset)
{
    void* mapping = ::mmap(nullptr, length, protection, flags, fd, static_cast<off_t>(offset));
    if (mapping == MAP_FAILED)
    {
        int err = errno;
        const char* reason = ::explain_mmap(nullptr, length, protection, flags, fd, static_cast<off_t>(offset));
        throw system_error(reason, err);
    }
    return mapping;
}

// We have to use a template because the compiler won't convert T* to void*&.
template <typename T>
inline void unmap_fd(T*& addr, size_t length)
{
    if (addr)
    {
        void* tmp = addr;
        addr = nullptr;
        if (-1 == ::munmap(tmp, length))
        {
            int err = errno;
            const char* reason = ::explain_munmap(tmp, length);
            throw system_error(reason, err);
        }
    }
}

} // namespace common
} // namespace gaia
