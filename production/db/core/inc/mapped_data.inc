/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

template <typename T>
base_mapped_data_t<T>::base_mapped_data_t()
{
    clear();
}

template <typename T>
base_mapped_data_t<T>::~base_mapped_data_t()
{
    close();
}

template <typename T>
void base_mapped_data_t<T>::clear()
{
    m_is_set = false;
    m_fd = -1;
    m_data = nullptr;
    m_mapped_data_size = 0;
}

template <typename T>
void base_mapped_data_t<T>::reset(base_mapped_data_t<T>& other)
{
    common::retail_assert(
        !m_is_set,
        "A set base_mapped_data_t instance should not take ownership of another instance!");
    common::retail_assert(
        other.m_is_set,
        "An unset base_mapped_data_t instance should not take ownership of another unset instance!");

    m_is_set = other.m_is_set;
    m_fd = other.m_fd;
    m_data = other.m_data;
    m_mapped_data_size = other.m_mapped_data_size;

    other.clear();
}

template <typename T>
void base_mapped_data_t<T>::close()
{
    common::unmap_fd_data(m_data, m_mapped_data_size);
    m_mapped_data_size = 0;

    common::close_fd(m_fd);

    m_is_set = false;
}

template <typename T>
T* base_mapped_data_t<T>::data()
{
    return m_data;
}

template <typename T>
int base_mapped_data_t<T>::fd()
{
    return m_fd;
}

template <typename T>
bool base_mapped_data_t<T>::is_set()
{
    return m_is_set;
}

template <typename T>
void mapped_data_t<T>::create(const char* name)
{
    common::retail_assert(
        !this->m_is_set,
        "Calling create() on an already set mapped_data_t instance!");

    this->m_fd = ::memfd_create(name, MFD_ALLOW_SEALING);
    if (this->m_fd == -1)
    {
        common::throw_system_error("memfd_create() failed in mapped_data_t::create()!");
    }

    this->m_mapped_data_size = sizeof(T);

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
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_NORESERVE,
        this->m_fd,
        0);

    this->m_is_set = true;
}

template <typename T>
void mapped_data_t<T>::open(int fd, bool manage_fd)
{
    common::retail_assert(
        !this->m_is_set,
        "Calling open() on an already set mapped_data_t instance!");

    common::retail_assert(fd != -1, "mapped_data_t::open() was called with an invalid fd!");

    this->m_mapped_data_size = sizeof(T);

    if (manage_fd)
    {
        this->m_fd = fd;

        common::map_fd_data(
            this->m_data,
            this->m_mapped_data_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_NORESERVE,
            this->m_fd,
            0);

        common::close_fd(this->m_fd);
    }
    else
    {
        common::map_fd_data(
            this->m_data,
            this->m_mapped_data_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_NORESERVE,
            fd,
            0);
    }

    this->m_is_set = true;
}
