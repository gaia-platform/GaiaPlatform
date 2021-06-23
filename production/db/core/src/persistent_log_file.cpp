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
persistent_log_file_t::persistent_log_file_t(std::string& dir, int fd, persistent_log_sequence_t file_seq, size_t size)
{
    persistent_log_file_t::dir_fd = fd;
    persistent_log_file_t::dir_name = dir;
    persistent_log_file_t::file_num = file_seq;
    persistent_log_file_t::file_size = size;

    // Open and fallocate depending on size.
    std::stringstream file_name;
    file_name << dir_name << "/" << file_num;
    file_fd = open(file_name.str().c_str(), O_WRONLY | O_CREAT, 0666);
    if (file_fd < 0)
    {
        throw_system_error("Unable to create wal file", errno);
    }

    // Todo: zero-fill entires in file.
    // Reference: http://yoshinorimatsunobu.blogspot.com/2009/05/overwriting-is-much-faster-than_28.html
    auto res = fallocate(file_fd, 0, 0, file_size);
    if (res != 0)
    {
        throw_system_error("Fallocate failed", errno);
    }

    res = fsync(file_fd);
    std::cout << "FILE CREATED = " << file_fd << std::endl;
    if (res != 0)
    {
        throw_system_error("Fsync when creating new log file failed.", errno);
    }

    // Calling fsync() does not necessarily ensure that the entry in the directory containing
    // the file has also reached disk. For that an explicit fsync() on a file descriptor
    // for the directory is also needed.
    res = fsync(dir_fd);
    if (res != 0)
    {
        throw_system_error("Fsync on persistent log directory failed.", errno);
    }
}

persistent_log_file_offset_t persistent_log_file_t::get_current_offset()
{
    return current_offset;
}

void persistent_log_file_t::allocate(size_t size)
{
    current_offset += size;
}

bool persistent_log_file_t::has_enough_space(size_t record_size)
{
    return current_offset + record_size <= file_size;
}

// Just for testing.
std::string persistent_log_file_t::get_file_name()
{
    std::string ret = dir_name;
    ret.append("/").append(std::to_string(file_num));
    return ret;
}
