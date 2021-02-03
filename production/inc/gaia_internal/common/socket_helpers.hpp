/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <iostream>
#include <ostream>
#include <stdexcept>

#include <sys/socket.h>
#include <sys/un.h>

#include "gaia/exception.hpp"
#include "retail_assert.hpp"
#include "system_error.hpp"

namespace gaia
{
namespace common
{

// 1K oughta be enough for anybody...
constexpr size_t c_max_msg_size = 1 << 10;

// This is the value of SCM_MAX_FD according to the manpage for unix(7).
// constexpr size_t c_max_fd_count = 253;
// Set to a reasonable value for a stack buffer (eventually helper functions
// will be templated on size).
constexpr size_t c_max_fd_count = 16;

// We throw this exception on either EPIPE/SIGPIPE caught from a write
// or EOF returned from a read (where a 0-length read is impossible).
class peer_disconnected : public gaia_exception
{
public:
    peer_disconnected()
    {
        m_message = "The socket peer is disconnected.";
    }
};

inline void check_socket_type(int socket, int expected_socket_type)
{
    int real_socket_type;
    socklen_t type_len = sizeof(real_socket_type);
    if (-1 == ::getsockopt(socket, SOL_SOCKET, SO_TYPE, &real_socket_type, &type_len))
    {
        throw_system_error("getsockopt(SO_TYPE) failed");
    }
    // type_len is an inout parameter which can indicate truncation.
    retail_assert(type_len == sizeof(real_socket_type), "Invalid socket type size!");
    retail_assert(real_socket_type == expected_socket_type, "Unexpected socket type!");
}

inline bool is_non_blocking(int socket)
{
    int flags = ::fcntl(socket, F_GETFL, 0);
    if (flags == -1)
    {
        throw_system_error("fcntl(F_GETFL) failed");
    }
    return (flags & O_NONBLOCK);
}

inline void set_non_blocking(int socket)
{
    int flags = ::fcntl(socket, F_GETFL);
    if (flags == -1)
    {
        throw_system_error("fcntl(F_GETFL) failed");
    }
    if (-1 == ::fcntl(socket, F_SETFL, flags | O_NONBLOCK))
    {
        throw_system_error("fcntl(F_SETFL) failed");
    }
}

inline size_t send_msg_with_fds(int sock, const int* fds, size_t fd_count, void* data, size_t data_size)
{
    // We should never send 0 bytes of data (because that would make it impossible
    // to determine if a 0-byte read meant EOF).
    retail_assert(data_size > 0, "Invalid data size!");
    // fd_count has value equal to length of fds array,
    // and all fds we send must fit in control.buf below.
    if (fds)
    {
        retail_assert(fd_count && fd_count <= c_max_fd_count, "Invalid fds!");
    }

    struct msghdr msg = {0};
    struct iovec iov = {0};
    // This is a union only to guarantee alignment for cmsghdr.
    union
    {
        // This is a dummy field for alignment only.
        struct cmsghdr dummy;
        char buf[CMSG_SPACE(sizeof(int) * c_max_fd_count)];
    } control;
    ::memset(&control.buf, 0, sizeof(control.buf));
    iov.iov_base = data;
    iov.iov_len = data_size;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = nullptr;
    msg.msg_controllen = 0;
    if (fds)
    {
        msg.msg_control = control.buf;
        msg.msg_controllen = CMSG_SPACE(sizeof(int) * fd_count);
        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); // NOLINT (macro expansion)
        cmsg->cmsg_len = CMSG_LEN(sizeof(int) * fd_count);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        for (size_t i = 0; i < fd_count; i++)
        {
            (reinterpret_cast<int*>(CMSG_DATA(cmsg)))[i] = fds[i];
        }
    }

    // Linux apparently doesn't send SIGPIPE on a client disconnect,
    // but POSIX requires it, so setting MSG_NOSIGNAL to be sure.
    // On BSD platforms we could use the SO_SIGNOPIPE socket option,
    // otherwise we need to suppress SIGPIPE (portable code here:
    // https://github.com/kroki/XProbes/blob/1447f3d93b6dbf273919af15e59f35cca58fcc23/src/libxprobes.c#L156).
    ssize_t bytes_written_or_error = ::sendmsg(sock, &msg, MSG_NOSIGNAL);
    // Since we assert that we never send 0 bytes, we should never return 0 bytes written.
    retail_assert(
        bytes_written_or_error != 0,
        "sendmsg() should never return 0 bytes written unless we write 0 bytes.");
    retail_assert(
        bytes_written_or_error >= -1,
        "sendmsg() should never return a negative value except for -1.");
    if (bytes_written_or_error == -1)
    {
        if (errno == EPIPE)
        {
            throw peer_disconnected();
        }
        else
        {
            throw_system_error("sendmsg failed");
        }
    }
    auto bytes_written = static_cast<size_t>(bytes_written_or_error);
    retail_assert(
        bytes_written == data_size,
        "sendmsg() payload was truncated but we didn't get EMSGSIZE.");

    return bytes_written;
}

inline size_t recv_msg_with_fds(
    int sock, int* fds, size_t* pfd_count, void* data,
    size_t data_size, bool throw_on_zero_bytes_read = true)
{
    // *pfd_count has initial value equal to length of fds array,
    // and all fds we receive must fit in control.buf below.
    if (fds)
    {
        retail_assert(
            pfd_count && *pfd_count && *pfd_count <= c_max_fd_count,
            "Illegal size of fds array!");
    }
    struct msghdr msg = {0};
    struct iovec iov = {0};
    // This is a union only to guarantee alignment for cmsghdr.
    union
    {
        // This is a dummy field for alignment only.
        struct cmsghdr dummy;
        char buf[CMSG_SPACE(sizeof(int) * c_max_fd_count)];
    } control;
    ::memset(&control.buf, 0, sizeof(control.buf));
    iov.iov_base = data;
    iov.iov_len = data_size;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = nullptr;
    msg.msg_controllen = 0;
    if (fds)
    {
        msg.msg_control = control.buf;
        msg.msg_controllen = CMSG_SPACE(sizeof(int) * *pfd_count);
    }
    ssize_t bytes_read = ::recvmsg(sock, &msg, 0);
    retail_assert(
        bytes_read >= -1,
        "recvmsg() should never return a negative value except for -1.");
    if (bytes_read == -1)
    {
        throw_system_error("recvmsg failed");
    }
    else if (bytes_read == 0)
    {
        // For seqpacket and datagram sockets, we cannot distinguish between a
        // zero-length datagram and a disconnected client, so we assume the
        // application protocol doesn't allow zero-length datagrams. If the
        // client specifies `throw_on_zero_bytes_read = false`, we assume it is
        // expecting EOF, so we just return 0 bytes read through the normal
        // codepath.
        if (throw_on_zero_bytes_read)
        {
            throw peer_disconnected();
        }
    }
    else if (msg.msg_flags & MSG_TRUNC)
    {
        throw system_error(
            "recvmsg: data payload truncated on read");
    }
    else if (msg.msg_flags & MSG_CTRUNC)
    {
        throw system_error(
            "recvmsg: control payload truncated on read");
    }

    if (fds)
    {
        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); // NOLINT (macro expansion)
        if (cmsg)
        {
            // If `throw_on_zero_bytes_read == false`, we assume that a
            // zero-length data read means the client interprets it as EOF. In
            // that case, the server shouldn't try to attach any fds, even
            // though Linux (unlike POSIX) can apparently attach ancillary data
            // to a zero-length datagram. All our existing protocols assume that
            // an empty datagram signifies EOF and the client should expect no
            // ancillary data to be attached.
            retail_assert(
                bytes_read > 0,
                "Ancillary data was attached to a zero-length datagram!");
            // message contains some fds, extract them
            retail_assert(cmsg->cmsg_level == SOL_SOCKET, "Invalid message header level!");
            retail_assert(cmsg->cmsg_type == SCM_RIGHTS, "Invalid message header type!");
            // This potentially fails to account for padding after cmsghdr,
            // but seems to work in practice, and there's no supported way
            // to directly get this information.
            size_t fd_count = (cmsg->cmsg_len - sizeof(struct cmsghdr)) / sizeof(int);
            // *pfd_count has initial value equal to length of fds array
            retail_assert(fd_count <= *pfd_count, "Mismatched fd count!");
            for (size_t i = 0; i < fd_count; i++)
            {
                fds[i] = (reinterpret_cast<int*>(CMSG_DATA(cmsg)))[i];
            }
            // *pfd_count has final value equal to number of fds returned
            *pfd_count = fd_count;
        }
        else
        {
            // message contains no fds, notify caller
            *pfd_count = 0;
        }
    }
    return static_cast<size_t>(bytes_read);
}

} // namespace common
} // namespace gaia
