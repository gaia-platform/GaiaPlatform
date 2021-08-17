/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "mapped_data.hpp"

namespace gaia
{
namespace db
{

void mapped_log_t::create(const char* name)
{
    ASSERT_PRECONDITION(
        !this->m_is_set,
        "Calling create() on an already set mapped_log_t instance!");

    this->m_fd = ::memfd_create(name, MFD_ALLOW_SEALING);
    if (this->m_fd == -1)
    {
        common::throw_system_error("memfd_create() failed in mapped_log_t::create()!");
    }

    this->m_mapped_data_size = c_initial_log_size;

    common::truncate_fd(this->m_fd, this->m_mapped_data_size);

    common::map_fd_data(
        this->m_data,
        this->m_mapped_data_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        this->m_fd,
        0);

    this->m_is_set = true;
}

void mapped_log_t::open(int fd, bool read_only)
{
    ASSERT_PRECONDITION(
        !this->m_is_set,
        "Calling open() on an already set mapped_log_t instance!");

    ASSERT_PRECONDITION(fd != -1, "mapped_log_t::open() was called with an invalid fd!");

    // We deliberately do not assign fd to this->m_fd, because we don't need ownership of the fd.
    // (The memory mapping retains an implicit reference to the memfd object until it's destroyed,
    // so callers can safely close the fd after they call this method.)
    this->m_mapped_data_size = common::get_fd_size(fd);

    common::map_fd_data(
        this->m_data,
        this->m_mapped_data_size,
        read_only ? PROT_READ : (PROT_READ | PROT_WRITE),
        // The MAP_PRIVATE flag is only required because of some undocumented
        // behavior I discovered in Linux: a memfd sealed for writes cannot be
        // mapped with MAP_SHARED, even if PROT_READ is specified. We need to
        // specify MAP_PRIVATE, even though we don't need a CoW mapping.
        read_only ? MAP_PRIVATE : MAP_SHARED,
        fd,
        0);

    this->m_is_set = true;
}

void mapped_log_t::open_shared(int)
{
    ASSERT_UNREACHABLE("This method should never be called on a mapped_log_t instance!");
}

int mapped_log_t::unmap_truncate_seal_fd()
{
    ASSERT_PRECONDITION(
        this->m_is_set,
        "Calling unmap_truncate_seal_fd() on an unset mapped_log_t instance!");

    ASSERT_PRECONDITION(
        this->m_fd != -1,
        "unmap_truncate_seal_fd() was called on a mapped_log_t instance that lacks a valid fd!");

    size_t actual_log_size = this->m_data->size();
    common::unmap_fd_data(this->m_data, this->m_mapped_data_size);
    this->m_mapped_data_size = 0;

    common::truncate_fd(this->m_fd, actual_log_size);

    // Seal the txn log memfd to prevent writes and resizing.
    if (-1 == ::fcntl(this->m_fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE))
    {
        common::throw_system_error(
            "fcntl(F_ADD_SEALS) failed in mapped_log_t::unmap_truncate_seal_fd()!");
    }

    // We give up our ownership of the log fd, although it is still valid.
    int fd = this->m_fd;
    this->m_fd = -1;
    this->m_is_set = false;

    return fd;
}

} // namespace db
} // namespace gaia
