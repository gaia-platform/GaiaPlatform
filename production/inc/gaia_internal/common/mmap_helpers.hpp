/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stdexcept>

#include <libexplain/mmap.h>
#include <libexplain/munmap.h>
#include <sys/mman.h>

#include "gaia/exception.hpp"

#include "gaia_internal/common/system_error.hpp"

namespace gaia
{
namespace common
{

// We use a template to avoid a cast from void* in the caller.
template <typename T>
inline void map_fd(T*& addr, size_t length, int protection, int flags, int fd, size_t offset)
{
    void* mapping = ::mmap(nullptr, length, protection, flags, fd, static_cast<off_t>(offset));
    if (mapping == MAP_FAILED)
    {
        int err = errno;
        const char* reason = ::explain_mmap(nullptr, length, protection, flags, fd, static_cast<off_t>(offset));
        throw system_error(reason, err);
    }
    addr = static_cast<T*>(mapping);
}

// We use a template because the compiler won't convert T* to void*&.
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

template <typename T>
class mmapped_data_t
{
public:
    mmapped_data_t()
    {
        m_data = nullptr;
        m_fd = -1;
    }

    ~mmapped_data_t()
    {
        close();
    }

    void open(const char* name)
    {
        m_fd = ::memfd_create(name, MFD_ALLOW_SEALING);
        if (m_fd == -1)
        {
            throw_system_error("memfd_create() failed!");
        }

        truncate_fd(m_fd, sizeof(T));

        // Note that unless we supply the MAP_POPULATE flag to mmap(), only the
        // pages we actually use will ever be allocated. However, Linux may refuse
        // to allocate the virtual memory we requested if overcommit is disabled
        // (i.e., /proc/sys/vm/overcommit_memory = 2). Using MAP_NORESERVE (don't
        // reserve swap space) should ensure that overcommit always succeeds, unless
        // it is disabled. We may need to document the requirement that overcommit
        // is not disabled (i.e., /proc/sys/vm/overcommit_memory != 2).
        //
        // Alternatively, we could use the more tedious but reliable approach of
        // using mmap(PROT_NONE) and calling mprotect(PROT_READ|PROT_WRITE) on any
        // pages we need to use (this is analogous to VirtualAlloc(MEM_RESERVE)
        // followed by VirtualAlloc(MEM_COMMIT) in Windows).
        map_fd(m_data, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, m_fd, 0);
    }

    void close()
    {
        unmap_fd(m_data, sizeof(T));
        close_fd(m_fd);
    }

    T* data()
    {
        return m_data;
    }

    int fd()
    {
        return m_fd;
    }

    bool is_initialized()
    {
        return m_data != nullptr || m_fd != -1;
    }

private:
    T* m_data;
    int m_fd;
};

} // namespace common
} // namespace gaia
