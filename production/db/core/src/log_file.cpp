/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "log_file.hpp"

#include <string>

#include <libexplain/fsync.h>
#include <libexplain/openat.h>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace persistence
{

// TODO (Mihir): Use io_uring for fsync, close & fallocate operations in this file.
// open() operation will remain synchronous, since we need the file fd to perform other async
// operations on the file.
log_file_t::log_file_t(const std::string& dir, int dir_fd, log_sequence_t log_seq, size_t size)
{
    m_dir_fd = dir_fd;
    m_dir_name = dir;
    m_log_seq = log_seq;
    m_file_size = size - c_reserved_size;
    m_current_offset = 0;

    // open and fallocate depending on size.
    std::stringstream file_name;
    file_name << m_dir_name << "/" << m_log_seq;
    m_file_fd = openat(dir_fd, file_name.str().c_str(), O_WRONLY | O_CREAT, c_file_permissions);
    if (m_file_fd < 0)
    {
        const char* reason = ::explain_openat(dir_fd, file_name.str().c_str(), O_WRONLY | O_CREAT, c_file_permissions);
        throw_system_error(reason);
    }

    // Reference: http://yoshinorimatsunobu.blogspot.com/2009/05/overwriting-is-much-faster-than_28.html
    if (-1 == fallocate(m_file_fd, FALLOC_FL_KEEP_SIZE, 0, m_file_size))
    {
        throw_system_error("fallocate() when creating persistent log file failed.");
    }

    // Call fsync to persist file metadata after the fallocate call.
    if (-1 == fsync(m_file_fd))
    {
        const char* reason = ::explain_fsync(m_file_fd);
        throw_system_error(reason);
    }

    // Calling fsync() on the file fd does not ensure that the entry in the directory containing
    // the file has also reached disk. For that an explicit fsync() on a file descriptor
    // for the directory is also needed.
    if (-1 == fsync(m_dir_fd))
    {
        const char* reason = ::explain_fsync(m_dir_fd);
        throw_system_error(reason);
    }
}

file_offset_t log_file_t::get_current_offset()
{
    return m_current_offset;
}

int log_file_t::get_file_fd()
{
    return m_file_fd;
}

void log_file_t::allocate(size_t size)
{
    m_current_offset += size;
}

log_sequence_t log_file_t::get_log_sequence()
{
    return m_log_seq;
}

size_t log_file_t::get_bytes_remaining_after_append(size_t record_size)
{
    ASSERT_INVARIANT(m_file_size > 0, "Preallocated file size should be greater than 0.");
    if (m_file_size < (m_current_offset + record_size))
    {
        return 0;
    }
    return m_file_size - (m_current_offset + record_size);
}

} // namespace persistence
} // namespace db
} // namespace gaia
