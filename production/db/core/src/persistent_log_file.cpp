/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "persistent_log_file.hpp"

#include <fcntl.h>

#include <cstddef>

#include <atomic>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/write_ahead_log_error.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "persistence_types.hpp"

using namespace gaia::common;
using namespace gaia::db;

// Todo (Mihir): Use io_uring for fsync, close & fallocate operations in this file.
// Open() operation will remain synchronous, since we need the file fd to perform other async
// operations on the file.
persistent_log_file_t::persistent_log_file_t(const std::string& dir, int dir_fd, size_t file_seq, size_t size)
{
    m_dir_fd = dir_fd;
    m_dir_name = dir;
    m_file_num = file_seq;
    m_file_size = size;
    m_current_offset = 0;
    std::cout << "REQUESTED_FILE SIZE BYTES = " << size << std::endl;

    // Open and fallocate depending on size.
    std::stringstream file_name;
    file_name << m_dir_name << "/" << m_file_num;
    m_file_fd = openat(dir_fd, file_name.str().c_str(), O_WRONLY | O_CREAT, c_file_permissions);
    if (m_file_fd < 0)
    {
        throw_system_error("openat() when creating persistent log file failed.");
    }

    // Todo: zero-fill entires in file.
    // Reference: http://yoshinorimatsunobu.blogspot.com/2009/05/overwriting-is-much-faster-than_28.html
    auto res = fallocate(m_file_fd, 0, 0, m_file_size);
    if (res != 0)
    {
        throw_system_error("fallocate() when creating persistent log file failed.");
    }

    res = fsync(m_file_fd);
    std::cout << "FILE CREATED " << std::endl;
    if (res != 0)
    {
        throw_system_error("fsync() when creating persistent log file failed.");
    }

    // Calling fsync() does not necessarily ensure that the entry in the directory containing
    // the file has also reached disk. For that an explicit fsync() on a file descriptor
    // for the directory is also needed.
    res = fsync(m_dir_fd);
    if (res != 0)
    {
        throw_system_error("fsync() on persistent log directory failed.");
    }
}

persistent_log_file_offset_t persistent_log_file_t::get_current_offset()
{
    return m_current_offset;
}

int persistent_log_file_t::get_file_fd()
{
    return m_file_fd;
}

void persistent_log_file_t::allocate(size_t size)
{
    m_current_offset += size;
}

size_t persistent_log_file_t::get_remaining_space(size_t record_size)
{
    ASSERT_PRECONDITION(m_file_size > 0, "Preallocated file size should be greater than 0.");
    std::cout << "CURRENT OFFSET = " << m_current_offset << std::endl;
    std::cout << "RECORD_SIZE = " << record_size << std::endl;
    return m_file_size - (m_current_offset + record_size);
}

// Just for testing.
void persistent_log_file_t::get_file_name(std::string& file_name)
{
    std::stringstream file_name0;
    file_name0 << m_dir_name << "/" << m_file_num;
    file_name = file_name0.str();
}

uint64_t persistent_log_file_t::get_log_file_seq()
{
    return m_file_num;
}
