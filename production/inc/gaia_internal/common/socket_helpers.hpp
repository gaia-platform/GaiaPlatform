/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stdexcept>

#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gaia/exception.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_error.hpp"

namespace gaia
{
namespace common
{

// This is exactly 1 page (4KB).
// On Linux, Unix domain socket datagrams must reside in contiguous physical memory, so sending a
// datagram larger than 1 page could fail if the OS cannot allocate enough contiguous physical
// pages.
// See https://stackoverflow.com/questions/4729315/what-is-the-max-size-of-af-unix-datagram-message-in-linux.
constexpr size_t c_max_msg_size_in_bytes{1 << 12};

// This could be up to 253 (the value of SCM_MAX_FD according to the manpage for unix(7)), but we
// set it to a reasonable value for a stack buffer.
// TODO: Add max fd count template parameter to socket helper functions.
constexpr size_t c_max_fd_count{16};

/**
 * Thrown on either EPIPE or ECONNRESET returned from a write
 * or EOF returned from a read (where a 0-length read is impossible).
 */
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
        throw_system_error("getsockopt(SO_TYPE) failed!");
    }
    // type_len is an inout parameter which can indicate truncation.
    ASSERT_POSTCONDITION(type_len == sizeof(real_socket_type), "Invalid socket type size!");
    ASSERT_POSTCONDITION(real_socket_type == expected_socket_type, "Unexpected socket type!");
}

inline bool is_non_blocking(int socket)
{
    int flags = ::fcntl(socket, F_GETFL, 0);
    if (flags == -1)
    {
        throw_system_error("fcntl(F_GETFL) failed!");
    }
    return (flags & O_NONBLOCK);
}

inline void set_non_blocking(int socket)
{
    int flags = ::fcntl(socket, F_GETFL);
    if (flags == -1)
    {
        throw_system_error("fcntl(F_GETFL) failed!");
    }
    if (-1 == ::fcntl(socket, F_SETFL, flags | O_NONBLOCK))
    {
        throw_system_error("fcntl(F_SETFL) failed!");
    }
}

inline size_t send_msg_with_fds(int sock, const int* fds, size_t fd_count, void* data, size_t data_size)
{
    // We should never send 0 bytes of data (because that would make it impossible
    // to determine if a 0-byte read meant EOF).
    ASSERT_PRECONDITION(data_size > 0, "Invalid data size!");
    // fd_count has value equal to length of fds array,
    // and all fds we send must fit in control.buf below.
    if (fds)
    {
        ASSERT_PRECONDITION(fd_count && fd_count <= c_max_fd_count, "Invalid fds!");
    }

    msghdr msg{};
    iovec iov{};
    // This is a union only to guarantee alignment for cmsghdr.
    union
    {
        // This is a dummy field for alignment only.
        cmsghdr dummy;
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
        cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); // NOLINT (macro expansion)
        ASSERT_INVARIANT(cmsg != nullptr, "CMSG_FIRSTHDR() should return a non-null pointer!")
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
    ASSERT_INVARIANT(
        bytes_written_or_error != 0,
        "sendmsg() should never return 0 bytes written unless we write 0 bytes.");
    ASSERT_INVARIANT(
        bytes_written_or_error >= -1,
        "sendmsg() should never return a negative value except for -1.");
    if (bytes_written_or_error == -1)
    {
        // On Linux, apparently a socket write should trigger ECONNRESET only
        // when the peer has closed its end of the socket, there is still
        // pending outgoing data in the writer's socket buffer, and the socket
        // is a TCP socket, but we assume this could happen for Unix domain
        // sockets as well, given that the exact behavior is undocumented and
        // therefore could change.
        // See https://stackoverflow.com/questions/2974021/what-does-econnreset-mean-in-the-context-of-an-af-local-socket.
        if (errno == EPIPE || errno == ECONNRESET)
        {
            throw peer_disconnected();
        }
        else
        {
            throw_system_error("sendmsg() failed!");
        }
    }
    auto bytes_written = static_cast<size_t>(bytes_written_or_error);
    ASSERT_POSTCONDITION(
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
        ASSERT_PRECONDITION(
            pfd_count && *pfd_count && *pfd_count <= c_max_fd_count,
            "Illegal size of fds array!");
    }
    msghdr msg{};
    iovec iov{};
    // This is a union only to guarantee alignment for cmsghdr.
    union
    {
        // This is a dummy field for alignment only.
        cmsghdr dummy;
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
    ASSERT_INVARIANT(
        bytes_read >= -1,
        "recvmsg() should never return a negative value except for -1.");
    if (bytes_read == -1)
    {
        // On Linux, apparently a read on a Unix domain socket can trigger
        // ECONNRESET if the peer has closed its end of the socket and there is
        // still pending outgoing data in the reader's socket buffer.
        // See https://stackoverflow.com/questions/2974021/what-does-econnreset-mean-in-the-context-of-an-af-local-socket.
        if (errno == ECONNRESET)
        {
            throw peer_disconnected();
        }
        throw_system_error("recvmsg() failed!");
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
            "recvmsg: data payload truncated on read.");
    }
    else if (msg.msg_flags & MSG_CTRUNC)
    {
        throw system_error(
            "recvmsg: control payload truncated on read.");
    }

    if (fds)
    {
        cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); // NOLINT (macro expansion)
        if (cmsg)
        {
            // If `throw_on_zero_bytes_read == false`, we assume that a
            // zero-length data read means the client interprets it as EOF. In
            // that case, the server shouldn't try to attach any fds, even
            // though Linux (unlike POSIX) can apparently attach ancillary data
            // to a zero-length datagram. All our existing protocols assume that
            // an empty datagram signifies EOF and the client should expect no
            // ancillary data to be attached.
            ASSERT_INVARIANT(
                bytes_read > 0,
                "Ancillary data was attached to a zero-length datagram!");
            // message contains some fds, extract them
            ASSERT_INVARIANT(cmsg->cmsg_level == SOL_SOCKET, "Invalid message header level!");
            ASSERT_INVARIANT(cmsg->cmsg_type == SCM_RIGHTS, "Invalid message header type!");
            // This potentially fails to account for padding after cmsghdr,
            // but seems to work in practice, and there's no supported way
            // to directly get this information.
            size_t fd_count = (cmsg->cmsg_len - sizeof(cmsghdr)) / sizeof(int);
            // *pfd_count has initial value equal to length of fds array
            ASSERT_INVARIANT(fd_count <= *pfd_count, "Mismatched fd count!");
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
