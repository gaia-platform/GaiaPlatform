/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <stdexcept>

#include <libexplain/close.h>
#include <libexplain/fstat.h>
#include <libexplain/ftruncate.h>
#include <sys/eventfd.h>
#include <sys/stat.h>

#include "gaia_exception.hpp"
#include "retail_assert.hpp"
#include "system_error.hpp"

namespace gaia
{
namespace common
{

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

inline void close_fd(int& fd)
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

inline void truncate_fd(int fd, size_t length)
{
    if (-1 == ::ftruncate(fd, static_cast<off_t>(length)))
    {
        int err = errno;
        const char* reason = ::explain_ftruncate(fd, static_cast<off_t>(length));
        throw system_error(reason, err);
    }
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
        throw_system_error("eventfd failed");
    }
    return eventfd;
}

inline void signal_eventfd(int eventfd)
{
    // from https://www.man7.org/linux/man-pages/man2/eventfd.2.html
    constexpr uint64_t MAX_SEMAPHORE_COUNT = std::numeric_limits<uint64_t>::max() - 1;
    // Signal the eventfd by writing a nonzero value.
    // This value is large enough that no thread will
    // decrement it to zero, so every waiting thread
    // should see a nonzero value.
    ssize_t bytes_written = ::write(eventfd, &MAX_SEMAPHORE_COUNT, sizeof(MAX_SEMAPHORE_COUNT));
    if (bytes_written == -1)
    {
        throw_system_error("write(eventfd) failed");
    }
    retail_assert(bytes_written == sizeof(MAX_SEMAPHORE_COUNT), "Failed to fully write data!");
}

inline void consume_eventfd(int eventfd)
{
    // We should always read the value 1 from a semaphore eventfd.
    uint64_t val;
    ssize_t bytes_read = ::read(eventfd, &val, sizeof(val));
    if (bytes_read == -1)
    {
        throw_system_error("read(eventfd) failed");
    }
    retail_assert(bytes_read == sizeof(val), "Failed to fully read data!");
    retail_assert(val == 1, "Unexpected value!");
}

} // namespace common
} // namespace gaia
