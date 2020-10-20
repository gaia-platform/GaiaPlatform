/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

template <typename T_data>
base_mapped_data_t<T_data>::base_mapped_data_t()
{
    clear();
}

template <typename T_data>
base_mapped_data_t<T_data>::~base_mapped_data_t()
{
    close();
}

template <typename T_data>
void base_mapped_data_t<T_data>::clear()
{
    m_is_set = false;
    m_fd = -1;
    m_data = nullptr;
    m_mapped_data_size = 0;
}

template <typename T_data>
void base_mapped_data_t<T_data>::reset(base_mapped_data_t<T_data>& other)
{
    ASSERT_PRECONDITION(
        !m_is_set,
        "A set base_mapped_data_t instance should not take ownership of another instance!");
    ASSERT_PRECONDITION(
        other.m_is_set,
        "An unset base_mapped_data_t instance should not take ownership of another unset instance!");

    m_is_set = other.m_is_set;
    m_fd = other.m_fd;
    m_data = other.m_data;
    m_mapped_data_size = other.m_mapped_data_size;

    other.clear();
}

template <typename T_data>
void base_mapped_data_t<T_data>::close()
{
    common::unmap_fd_data(m_data, m_mapped_data_size);
    m_mapped_data_size = 0;

    common::close_fd(m_fd);

    m_is_set = false;
}

template <typename T_data>
T_data* base_mapped_data_t<T_data>::data()
{
    return m_data;
}

template <typename T_data>
int base_mapped_data_t<T_data>::fd()
{
    return m_fd;
}

template <typename T_data>
bool base_mapped_data_t<T_data>::is_set()
{
    return m_is_set;
}

template <typename T_data>
void mapped_data_t<T_data>::create(const char* name, size_t size, int prot, int flags)
{
    ASSERT_PRECONDITION(
        !this->m_is_set,
        "Calling create() on an already set mapped_data_t instance!");

    this->m_fd = ::memfd_create(name, MFD_ALLOW_SEALING);
    if (this->m_fd == -1)
    {
        common::throw_system_error("memfd_create() failed in mapped_data_t::create()!");
    }

    this->m_mapped_data_size = size;

    common::truncate_fd(this->m_fd, this->m_mapped_data_size);

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
    common::map_fd_data(
        this->m_data,
        this->m_mapped_data_size,
        prot,
        flags,
        this->m_fd,
        0);

    this->m_is_set = true;
}

template <typename T_data>
void mapped_data_t<T_data>::open(int fd, bool manage_fd, size_t size, int prot, int flags)
{
    ASSERT_PRECONDITION(
        !this->m_is_set,
        "Calling open() on an already set mapped_data_t instance!");

    ASSERT_PRECONDITION(fd != -1, "mapped_data_t::open() was called with an invalid fd!");

    this->m_mapped_data_size = size;

    if (manage_fd)
    {
        this->m_fd = fd;

        common::map_fd_data(
            this->m_data,
            this->m_mapped_data_size,
            prot,
            flags,
            this->m_fd,
            0);

        common::close_fd(this->m_fd);
    }
    else
    {
        common::map_fd_data(
            this->m_data,
            this->m_mapped_data_size,
            prot,
            flags,
            fd,
            0);
    }

    this->m_is_set = true;
}
