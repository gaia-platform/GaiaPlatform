/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <limits>
#include <ostream>
#include <stdexcept>

#include <libexplain/close.h>
#include <libexplain/dup.h>
#include <libexplain/eventfd.h>
#include <libexplain/fstat.h>
#include <libexplain/ftruncate.h>
#include <libexplain/pread.h>
#include <libexplain/read.h>
#include <libexplain/write.h>
#include <sys/eventfd.h>
#include <sys/stat.h>

#include "gaia/exception.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_error.hpp"

namespace gaia
{
namespace common
{

/**
 * Thrown when a read returns fewer bytes than expected.
 */
class incomplete_read_error : public gaia_exception
{
public:
    explicit incomplete_read_error(size_t expected_bytes_read, size_t actual_bytes_read)
    {
        std::stringstream message;
        message
            << "Expected to read " << expected_bytes_read
            << " bytes, but actually read " << actual_bytes_read << " bytes.";
        m_message = message.str();
    }
};

inline size_t get_fd_size(int fd)
{
    struct stat st;
    if (-1 == ::fstat(fd, &st))
    {
        int err = errno;
        const char* reason = ::explain_fstat(fd, &st);
        throw system_error(reason, err);
    }
    return st.st_size;
}

// NB: this should only be used for asserts, not for ordinary program logic!
// Your program should never have to ask whether an fd is valid!
inline bool is_fd_valid(int fd)
{
    // Querying fd flags seems like the lowest-overhead way to check the
    // validity of an fd, since it only accesses the per-process fd table, not
    // the global open file table.
    return (::fcntl(fd, F_GETFD) != -1 || errno != EBADF);
}

inline bool is_fd_regular_file(int fd)
{
    struct stat st;
    if (-1 == ::fstat(fd, &st))
    {
        int err = errno;
        const char* reason = ::explain_fstat(fd, &st);
        throw system_error(reason, err);
    }
    return S_ISREG(st.st_mode);
}

inline void close_fd(int& fd)
{
    if (fd != -1)
    {
        int tmp = fd;
        fd = -1;
        if (-1 == ::close(tmp))
        {
            int err = errno;
            const char* reason = ::explain_close(tmp);
            throw system_error(reason, err);
        }
    }
}

inline void truncate_fd(int fd, size_t length)
{
    if (-1 == ::ftruncate(fd, static_cast<off_t>(length)))
    {
        int err = errno;
        const char* reason = ::explain_ftruncate(fd, static_cast<off_t>(length));
        throw system_error(reason, err);
    }
}

inline int duplicate_fd(int fd)
{
    int new_fd = ::dup(fd);
    if (new_fd == -1)
    {
        int err = errno;
        const char* reason = ::explain_dup(fd);
        throw system_error(reason, err);
    }
    return new_fd;
}

inline size_t read_fd_at_offset(
    int fd, size_t offset, void* buffer, size_t buffer_len, bool allow_partial_read = false)
{
    ssize_t bytes_read_or_error = ::pread(fd, buffer, buffer_len, offset);
    ASSERT_INVARIANT(bytes_read_or_error >= -1, "Return value of pread() should be non-negative or -1!");
    if (bytes_read_or_error == -1)
    {
        int err = errno;
        const char* reason = ::explain_pread(fd, buffer, buffer_len, offset);
        throw system_error(reason, err);
    }
    size_t bytes_read = static_cast<size_t>(bytes_read_or_error);
    if (!allow_partial_read)
    {
        if (bytes_read < buffer_len)
        {
            throw incomplete_read_error(buffer_len, bytes_read);
        }
    }
    return bytes_read;
}

inline int make_eventfd()
{
    // Create eventfd shutdown event.
    // Linux is non-POSIX-compliant and sometimes marks an fd as readable
    // from select/poll/epoll even when a subsequent read would block.
    // Therefore it's safest to always set an fd to nonblocking when it's
    // used with select/poll/epoll. However, a datagram socket will return
    // EAGAIN/EWOULDBLOCK on write if there's not enough space in the send
    // buffer to write the whole message. This shouldn't be an issue as long
    // as our send buffer is larger than any message, but we should assert
    // that writes never block when the socket is writable, just to be sure.
    // We really just want the semantics of a broadcast, level-triggered
    // "waitable flag", but the closest thing to that is semaphore mode, which
    // has the unwanted semantics of a decrement on each read (the eventfd stops
    // alerting when it is decremented to zero). So as a workaround, we write
    // the largest possible value to the eventfd to ensure that it is never
    // decremented to zero, no matter how many threads read (and decrement) the
    // value.
    int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
    if (eventfd == -1)
    {
        int err = errno;
        const char* reason = ::explain_eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
        throw system_error(reason, err);
    }
    return eventfd;
}

inline void signal_eventfd(int eventfd)
{
    // from https://www.man7.org/linux/man-pages/man2/eventfd.2.html
    constexpr uint64_t c_max_semaphore_count = std::numeric_limits<uint64_t>::max() - 1;
    // Signal the eventfd by writing a nonzero value.
    // This value is large enough that no thread will
    // decrement it to zero, so every waiting thread
    // should see a nonzero value.
    ssize_t bytes_written = ::write(eventfd, &c_max_semaphore_count, sizeof(c_max_semaphore_count));
    if (bytes_written == -1)
    {
        int err = errno;
        const char* reason = ::explain_write(eventfd, &c_max_semaphore_count, sizeof(c_max_semaphore_count));
        throw system_error(reason, err);
    }
    ASSERT_POSTCONDITION(bytes_written == sizeof(c_max_semaphore_count), "Failed to fully write data!");
}

inline void consume_eventfd(int eventfd)
{
    // We should always read the value 1 from a semaphore eventfd.
    uint64_t val;
    ssize_t bytes_read = ::read(eventfd, &val, sizeof(val));
    if (bytes_read == -1)
    {
        int err = errno;
        const char* reason = ::explain_read(eventfd, &val, sizeof(val));
        throw system_error(reason, err);
    }
    ASSERT_POSTCONDITION(bytes_read == sizeof(val), "Failed to fully read data!");
    ASSERT_POSTCONDITION(val == 1, "Unexpected value!");
}

} // namespace common
} // namespace gaia
