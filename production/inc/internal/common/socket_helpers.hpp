/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sys/socket.h>
#include <sys/un.h>

#include <stdexcept>

#include "gaia_exception.hpp"
#include "retail_assert.hpp"
#include "system_error.hpp"

namespace gaia {
namespace common {

// Current protocols never send or receive > 2 fds at a time.
const size_t MAX_FD_COUNT = 2;

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

inline size_t send_msg_with_fds(int sock, const int *fds, size_t fd_count, void *data,
                         size_t data_size) {
    // We should never send 0 bytes of data (because that would make it impossible
    // to determine if a 0-byte read meant EOF).
    retail_assert(data_size > 0);
    // fd_count has value equal to length of fds array,
    // and all fds we send must fit in control.buf below.
    if (fds) {
        retail_assert(fd_count && fd_count <= MAX_FD_COUNT);
    }

    struct msghdr msg;
    struct iovec iov;
    // This is a union only to guarantee alignment for cmsghdr.
    union {
        // this is a dummy field for alignment only
        struct cmsghdr dummy;
        char buf[CMSG_SPACE(sizeof(int) * MAX_FD_COUNT)];
    } control;
    iov.iov_base = data;
    iov.iov_len = data_size;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = nullptr;
    msg.msg_controllen = 0;
    if (fds) {
        msg.msg_control = control.buf;
        msg.msg_controllen = sizeof(control.buf);
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int) * fd_count);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        for (size_t i = 0; i < fd_count; i++) {
            (reinterpret_cast<int*>(CMSG_DATA(cmsg)))[i] = fds[i];
        }
    }

    // Linux apparently doesn't send SIGPIPE on a client disconnect,
    // but POSIX requires it, so setting MSG_NOSIGNAL to be sure.
    // On BSD platforms we could use the SO_SIGNOPIPE socket option,
    // otherwise we need to suppress SIGPIPE (portable code here:
    // https://github.com/kroki/XProbes/blob/1447f3d93b6dbf273919af15e59f35cca58fcc23/src/libxprobes.c#L156).
    ssize_t bytes_written_or_error = sendmsg(sock, &msg, MSG_NOSIGNAL);
    if (bytes_written_or_error == -1) {
        if (errno == EPIPE) {
            throw peer_disconnected();
        } else {
            throw_system_error("sendmsg failed");
        }
    }
    // Since we assert that we never send 0 bytes, we should never return 0 bytes written.
    retail_assert(bytes_written_or_error != 0,
        "sendmsg() should never return 0 bytes written unless we write 0 bytes.");
    retail_assert(bytes_written_or_error >= 0,
        "sendmsg() should never return a negative value except for -1.");
    size_t bytes_written = (size_t)bytes_written_or_error;
    retail_assert(bytes_written < data_size,
        "sendmsg() payload was truncated but we didn't get EMSGSIZE.");

    return bytes_written;
}

inline size_t recv_msg_with_fds(int sock, int *fds, size_t *pfd_count, void *data,
                         size_t data_size) {
    // *pfd_count has initial value equal to length of fds array,
    // and all fds we receive must fit in control.buf below.
    if (fds) {
        retail_assert(pfd_count && *pfd_count && *pfd_count <= MAX_FD_COUNT);
    }
    struct msghdr msg;
    struct iovec iov;
    // This is a union only to guarantee alignment for cmsghdr.
    union {
        // this is a dummy field for alignment only
        struct cmsghdr dummy;
        char buf[CMSG_SPACE(sizeof(int) * MAX_FD_COUNT)];
    } control;
    iov.iov_base = data;
    iov.iov_len = data_size;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = nullptr;
    msg.msg_controllen = 0;
    if (fds) {
        msg.msg_control = control.buf;
        msg.msg_controllen = sizeof(control.buf);
    }
    ssize_t bytes_read = recvmsg(sock, &msg, 0);
    if (bytes_read == -1) {
        throw_system_error("recvmsg failed");
    } else if (bytes_read == 0) {
        // For seqpacket and datagram sockets, we cannot distinguish between
        // a zero-length datagram and a disconnected client, so we assume the
        // application protocol doesn't allow zero-length datagrams.
        throw peer_disconnected();
    } else if (msg.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) {
        throw system_error(
            "recvmsg: control or data payload truncated on read");
    }
    if (fds) {
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg) {
            // message contains some fds, extract them
            retail_assert(cmsg->cmsg_level == SOL_SOCKET);
            retail_assert(cmsg->cmsg_type == SCM_RIGHTS);
            // This potentially fails to account for padding after cmsghdr,
            // but seems to work in practice, and there's no supported way
            // to directly get this information.
            size_t fd_count = (cmsg->cmsg_len - sizeof(struct cmsghdr)) / sizeof(int);
            // *pfd_count has initial value equal to length of fds array
            retail_assert(fd_count <= *pfd_count);
            for (size_t i = 0; i < fd_count; i++) {
                fds[i] = (reinterpret_cast<int*>(CMSG_DATA(cmsg)))[i];
            }
            // *pfd_count has final value equal to number of fds returned
            *pfd_count = fd_count;
        } else {
            // message contains no fds, notify caller
            *pfd_count = 0;
        }
    }
    return static_cast<size_t>(bytes_read);
}

} // namespace common
} // namespace gaia
