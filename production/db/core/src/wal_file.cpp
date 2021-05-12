/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "wal_file.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>

#include <atomic>
#include <memory>
#include <sstream>
#include <string>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/write_ahead_log_error.hpp"
#include "gaia_internal/db/db_types.hpp"

using namespace gaia::common;
using namespace gaia::db;

// Todo (Mihir): Use io_uring for fsync, close & fallocate operations in this file.
// Open() operation will remain synchronous, since we need the file fd to perform other async
// operations on the file.
wal_file_t::wal_file_t(std::string& dir, int fd, wal_sequence_t file_seq, size_t size)
{
    dir_fd = fd;
    dir_name = dir;
    file_num = file_seq;
    file_size = size;

    // Open and fallocate depending on size.
    std::stringstream file_name;
    file_name << dir_name << "/" << file_num;
    file_fd = open(file_name.str().c_str(), O_WRONLY | O_CREAT);
    if (file_fd == -1)
    {
        throw write_ahead_log_error("Unable to create wal file");
    }

    // Todo: zero-fill entires in file.
    // Reference: http://yoshinorimatsunobu.blogspot.com/2009/05/overwriting-is-much-faster-than_28.html
    int err = fallocate(file_fd, 0, 0, file_size);

    if (err == -1)
    {
        int err = errno;
        throw write_ahead_log_error("fallocate call for wal file failed", err);
    }

    // Calling fsync() does not necessarily ensure that the entry in the directory containing
    // the file has also reached disk. For that an explicit fsync() on a file descriptor
    // for the directory is also needed.
    fsync(file_fd);
    fsync(dir_fd);
}

wal_file_t::~wal_file_t()
{
    close(file_fd);
}

wal_file_offset_t wal_file_t::get_current_offset()
{
    return current_offset;
}

void wal_file_t::allocate(size_t size)
{
    current_offset += size;
}

bool wal_file_t::has_enough_space(size_t record_size)
{
    return current_offset + record_size < file_size;
}
