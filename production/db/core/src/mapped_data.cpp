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

void mapped_log_t::open(int fd)
{
    ASSERT_PRECONDITION(
        !this->m_is_set,
        "Calling open() on an already set mapped_log_t instance!");

    ASSERT_PRECONDITION(fd != -1, "mapped_log_t::open() was called with an invalid fd!");

    this->m_mapped_data_size = common::get_fd_size(fd);

    common::map_fd_data(
        this->m_data,
        this->m_mapped_data_size,
        PROT_READ,
        MAP_PRIVATE,
        fd,
        0);

    this->m_is_set = true;
}

void mapped_log_t::open_shared(int)
{
    ASSERT_UNREACHABLE("This method should never be called on a mapped_log_t instance!");
}

void mapped_log_t::truncate_seal_and_close(int& fd, size_t& log_size)
{
    ASSERT_PRECONDITION(
        this->m_is_set,
        "Calling truncate_seal_and_close() on an unset mapped_log_t instance!");

    ASSERT_PRECONDITION(
        this->m_fd != -1,
        "truncate_seal_and_close() was called on a mapped_log_t instance that lacks a valid fd!");

    fd = this->m_fd;
    log_size = this->m_data->size();

    common::unmap_fd_data(this->m_data, this->m_mapped_data_size);
    this->m_mapped_data_size = 0;

    common::truncate_fd(this->m_fd, log_size);

    // Seal the txn log memfd for writes/resizing before sending it to the server.
    if (-1 == ::fcntl(this->m_fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE))
    {
        common::throw_system_error(
            "fcntl(F_ADD_SEALS) failed in mapped_log_t::truncate_seal_and_close()!");
    }

    this->m_fd = -1;

    this->m_is_set = false;
}

} // namespace db
} // namespace gaia
