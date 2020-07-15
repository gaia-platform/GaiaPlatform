/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sys/mman.h>
#include <libexplain/mmap.h>
#include <libexplain/munmap.h>

#include <stdexcept>

#include "gaia_exception.hpp"
#include "system_error.hpp"

namespace gaia {
namespace common {

inline void* map_fd(size_t length, int protection, int flags, int fd, size_t offset)
{
    void* mapping = mmap(nullptr, length, protection, flags, fd, static_cast<off_t>(offset));
    if (mapping == MAP_FAILED) {
        const char* reason = explain_mmap(nullptr, length, protection, flags, fd, static_cast<off_t>(offset));
        throw system_error(reason);
    }
    return mapping;
}

inline void unmap_fd(void* addr, size_t length)
{
    if (-1 == munmap(addr, length)) {
        const char* reason = explain_munmap(addr, length);
        throw system_error(reason);
    }
}

} // namespace common
} // namespace gaia
