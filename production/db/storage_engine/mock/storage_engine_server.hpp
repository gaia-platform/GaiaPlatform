/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <csignal>
#include <thread>

#include "storage_engine.hpp"
#include "array_size.hpp"
#include "retail_assert.hpp"
#include "system_error.hpp"
#include "mmap_helpers.hpp"
#include "socket_helpers.hpp"
#include "messages_generated.h"

namespace gaia {
namespace db {

using namespace common;
using namespace messages;
using namespace flatbuffers;

class invalid_session_transition : public gaia_exception {
   public:
    invalid_session_transition(const string& message) : gaia_exception(message) {}
};

class server : private se_base {
   public:
    static void run();

   private:
    // FIXME: this really should be constexpr, but C++11 seems broken in that respect.
    static const uint64_t MAX_SEMAPHORE_COUNT;
    static int s_server_shutdown_event_fd;
    static int s_connect_socket;
    static std::mutex s_commit_lock;
    static int s_fd_data;
    static offsets* s_shared_offsets;
    thread_local static session_state_t s_session_state;
    thread_local static bool s_session_shutdown;
    thread_local static gaia_xid_t s_transaction_id;

    // inherited from se_base:
    // static int s_fd_offsets;
    // static data *s_data;
    // thread_local static log *s_log;
    // thread_local static int s_session_socket;

    // function pointer type that executes side effects of a state transition
    typedef void (*transition_handler_t)(int* fds, size_t fd_count, session_event_t event, session_state_t old_state, session_state_t new_state);
    static void handle_connect(int*, size_t, session_event_t, session_state_t, session_state_t);
    static void handle_begin_txn(int*, size_t, session_event_t, session_state_t, session_state_t);
    static void handle_rollback_txn(int*, size_t, session_event_t, session_state_t, session_state_t);
    static void handle_commit_txn(int*, size_t, session_event_t, session_state_t, session_state_t);
    static void handle_decide_txn(int*, size_t, session_event_t, session_state_t, session_state_t);
    static void handle_client_shutdown(int*, size_t, session_event_t, session_state_t, session_state_t);
    static void handle_server_shutdown(int*, size_t, session_event_t, session_state_t, session_state_t);

    struct transition_t {
        session_state_t new_state;
        transition_handler_t handler;
    };

    struct valid_transition_t {
        session_state_t state;
        session_event_t event;
        transition_t transition;
    };

    // DISCONNECTED (client has connected to server listening socket, authenticated, and requested session)
    // -> CONNECTED
    // CONNECTED (client datagram socket has been allocated, client thread has been started,
    // server has replied to client from listening socket with its session socket,
    // client has connected to server from that socket, server has replied with fds
    // for data/locator segments)
    // -> TXN_IN_PROGRESS (client called begin_transaction)
    // -> DISCONNECTING (server half-closed session socket)
    // -> DISCONNECTED (client closed or half-closed session socket)
    // TXN_IN_PROGRESS (client sent begin_transaction message to server, server replied)
    // -> TXN_COMMITTING (client called commit_transaction)
    // -> CONNECTED (client rolled back transaction)
    // TXN_COMMITTING (server decided to commit or abort transaction)
    // -> CONNECTED

    static constexpr valid_transition_t s_valid_transitions[] = {
        {session_state_t::DISCONNECTED, session_event_t::CONNECT, {session_state_t::CONNECTED, handle_connect}},
        {session_state_t::ANY, session_event_t::CLIENT_SHUTDOWN, {session_state_t::DISCONNECTED, handle_client_shutdown}},
        {session_state_t::CONNECTED, session_event_t::BEGIN_TXN, {session_state_t::TXN_IN_PROGRESS, handle_begin_txn}},
        {session_state_t::TXN_IN_PROGRESS, session_event_t::ROLLBACK_TXN, {session_state_t::CONNECTED, handle_rollback_txn}},
        {session_state_t::TXN_IN_PROGRESS, session_event_t::COMMIT_TXN, {session_state_t::TXN_COMMITTING, handle_commit_txn}},
        {session_state_t::TXN_COMMITTING, session_event_t::DECIDE_TXN_COMMIT, {session_state_t::CONNECTED, handle_decide_txn}},
        {session_state_t::TXN_COMMITTING, session_event_t::DECIDE_TXN_ABORT, {session_state_t::CONNECTED, handle_decide_txn}},
        {session_state_t::CONNECTED, session_event_t::SERVER_SHUTDOWN, {session_state_t::DISCONNECTING, handle_server_shutdown}},
    };

    static void apply_transition(session_event_t event, int* fds, size_t fd_count) {
        if (event == session_event_t::NOP) {
            return;
        }
        // "Wildcard" transitions (current state = session_state_t::ANY) must be listed after
        // non-wildcard transitions with the same event, or the latter will never be applied.
        for (size_t i = 0; i < array_size(s_valid_transitions); i++) {
            valid_transition_t t = s_valid_transitions[i];
            if (t.event == event && (t.state == s_session_state || t.state == session_state_t::ANY)) {
                // It would be nice to statically enforce this on the Transition struct.
                retail_assert(t.transition.new_state != session_state_t::ANY);
                session_state_t old_state = s_session_state;
                s_session_state = t.transition.new_state;
                if (t.transition.handler) {
                    t.transition.handler(fds, fd_count, event, old_state, s_session_state);
                }
                return;
            }
        }
        // If we get here, we haven't found any compatible transition.
        // TODO: consider propagating exception back to client?
        throw invalid_session_transition(
            "no allowed state transition from state '" +
            std::string(EnumNamesession_state_t(s_session_state)) +
            "' with event '" +
            std::string(EnumNamesession_event_t(event)) + "'");
    }

    static void build_server_reply(
        FlatBufferBuilder& builder,
        session_event_t event,
        session_state_t old_state,
        session_state_t new_state,
        gaia_xid_t transaction_id) {
        auto server_reply = Createserver_reply_t(builder, event, old_state, new_state, transaction_id);
        auto message = Createmessage_t(builder, any_message_t::reply, server_reply.Union());
        builder.Finish(message);
    }

    static void init_shared_memory() {
        retail_assert(s_fd_data == -1 && s_fd_offsets == -1);
        retail_assert(!s_data && !s_shared_offsets);
        s_fd_offsets = memfd_create(SCH_MEM_OFFSETS, MFD_ALLOW_SEALING);
        if (s_fd_offsets == -1) {
            throw_system_error("memfd_create failed");
        }
        s_fd_data = memfd_create(SCH_MEM_DATA, MFD_ALLOW_SEALING);
        if (s_fd_data == -1) {
            throw_system_error("memfd_create failed");
        }
        if (-1 == ftruncate(s_fd_offsets, sizeof(offsets)) || -1 == ftruncate(s_fd_data, sizeof(data))) {
            throw_system_error("ftruncate failed");
        }
        s_shared_offsets = static_cast<offsets*>(map_fd(sizeof(offsets),
            PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_offsets, 0));
        s_data = static_cast<data*>(map_fd(sizeof(data),
            PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_data, 0));
    }

    static sigset_t mask_signals() {
        sigset_t sigset;
        sigemptyset(&sigset);
        // TODO: do we want to handle SIGHUP differently at some point (e.g. reload config)?
        sigaddset(&sigset, SIGHUP);
        sigaddset(&sigset, SIGINT);
        sigaddset(&sigset, SIGTERM);
        sigaddset(&sigset, SIGQUIT);
        // Per POSIX, we must use pthread_sigmask() rather than sigprocmask()
        // in a multithreaded program.
        // REVIEW: should this be SIG_SETMASK?
        pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
        return sigset;
    }

    static void signal_handler(sigset_t sigset, int& signum) {
        // Wait until a signal is delivered.
        // REVIEW: do we have any use for sigwaitinfo()?
        sigwait(&sigset, &signum);
        // Signal the eventfd by writing a nonzero value.
        // This value is large enough that no thread will
        // decrement it to zero, so every waiting thread
        // should see a nonzero value.
        ssize_t bytes_written = write(
            s_server_shutdown_event_fd, &MAX_SEMAPHORE_COUNT, sizeof(MAX_SEMAPHORE_COUNT));
        retail_assert(bytes_written == sizeof(MAX_SEMAPHORE_COUNT) || bytes_written == -1);
        if (bytes_written == -1) {
            throw_system_error("write to eventfd failed");
        }
    }

    static int get_client_dispatch_fd() {
        // Launch a connection-based listening Unix socket on a well-known address.
        // We use SOCK_SEQPACKET to get connection-oriented *and* datagram semantics.
        // This socket needs to be nonblocking so we can use epoll to wait on the
        // shutdown eventfd as well (the connected sockets it spawns will inherit
        // nonblocking mode). Actually, nonblocking mode may not be visible in level-
        // triggered epoll mode, but it's very important for edge-triggered mode,
        // and it might be useful if we ever wanted to do partial reads (e.g. on a
        // stream socket), so it seems like the right choice.
        int connect_socket = socket(PF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
        if (connect_socket == -1) {
            throw_system_error("socket creation failed");
        }
        struct sockaddr_un server_addr = {0};
        server_addr.sun_family = AF_UNIX;
        // The socket name (minus its null terminator) needs to fit into the space
        // in the server address structure after the prefix null byte.
        retail_assert(sizeof(SERVER_CONNECT_SOCKET_NAME) - 1 <= sizeof(server_addr.sun_path) - 1);
        // We prepend a null byte to the socket name so the address is in the
        // (Linux-exclusive) "abstract namespace", i.e., not bound to the
        // filesystem.
        strncpy(&server_addr.sun_path[1], SERVER_CONNECT_SOCKET_NAME,
            sizeof(server_addr.sun_path) - 1);
        // The socket name is not null-terminated in the address structure, but
        // we need to add an extra byte for the null byte prefix.
        socklen_t server_addr_size =
            sizeof(server_addr.sun_family) + 1 + strlen(&server_addr.sun_path[1]);
        if (-1 == ::bind(connect_socket, (struct sockaddr*)&server_addr, server_addr_size)) {
            throw_system_error("bind failed");
        }
        if (-1 == listen(connect_socket, 0)) {
            throw_system_error("listen failed");
        }
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            throw_system_error("epoll_create1 failed");
        }
        int fds[] = {connect_socket, s_server_shutdown_event_fd};
        for (size_t i = 0; i < array_size(fds); i++) {
            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = fds[i];
            if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[i], &ev)) {
                throw_system_error("epoll_ctl failed");
            }
        }
        s_connect_socket = connect_socket;
        return epoll_fd;
    }

    static bool authenticate_client_socket(int socket) {
        struct ucred cred;
        socklen_t cred_len = sizeof(cred);
        if (-1 == getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len)) {
            throw_system_error("getsockopt(SO_PEERCRED) failed");
        }
        // Client must have same effective user ID as server.
        return (cred.uid == geteuid());
    }

    static void client_dispatch_handler() {
        int epoll_fd = get_client_dispatch_fd();
        auto cleanup_epoll_fd = scope_guard::make_scope_guard([epoll_fd]() {
            close(epoll_fd);
        });
        std::vector<std::thread> session_threads;
        auto cleanup_session_threads = scope_guard::make_scope_guard([&session_threads]() {
            for (std::thread& t : session_threads) {
                t.join();
            }
        });
        struct epoll_event events[2];
        while (true) {
            // Block forever (we will be notified of shutdown).
            int ready_fd_count = epoll_wait(epoll_fd, events, array_size(events), -1);
            if (ready_fd_count == -1) {
                throw_system_error("epoll_wait failed");
            }
            for (int i = 0; i < ready_fd_count; i++) {
                const epoll_event ev = events[i];
                // We never register for anything but EPOLLIN,
                // but EPOLLERR will always be delivered.
                if (ev.events & EPOLLERR) {
                    // This flag is unmaskable, so we don't need to register for it.
                    if (ev.data.fd == s_connect_socket) {
                        int error = 0;
                        socklen_t err_len = sizeof(error);
                        // Ignore errors getting error message and default to generic error message.
                        getsockopt(s_connect_socket, SOL_SOCKET, SO_ERROR, (void*)&error, &err_len);
                        throw_system_error("client socket error", error);
                    } else if (ev.data.fd == s_server_shutdown_event_fd) {
                        throw_system_error("shutdown eventfd error");
                    }
                }
                // At this point, we should only get EPOLLIN.
                retail_assert(ev.events == EPOLLIN);
                if (ev.data.fd == s_connect_socket) {
                    int session_socket = accept(s_connect_socket, NULL, NULL);
                    if (session_socket == -1) {
                        throw_system_error("accept failed");
                    }
                    if (authenticate_client_socket(session_socket)) {
                        session_threads.emplace_back(session_thread, session_socket);
                    } else {
                        close(session_socket);
                    }
                } else if (ev.data.fd == s_server_shutdown_event_fd) {
                    uint64_t val;
                    ssize_t bytes_read = read(s_server_shutdown_event_fd, &val, sizeof(val));
                    if (bytes_read == -1) {
                        throw_system_error("read failed");
                    }
                    // We should always read the value 1 from a semaphore eventfd.
                    retail_assert(bytes_read == sizeof(val) && val == 1);
                    return;
                } else {
                    // We don't monitor any other fds.
                    retail_assert(false);
                }
            }
        }
    }

    static void session_thread(int session_socket) {
        // REVIEW: how do we gracefully close the session socket?
        // Do we need to issue a nonblocking read() first?
        // Then do we need to call shutdown() before close()?
        // With TCP, shutdown() will send a FIN packet, but what
        // does it do for a Unix socket? Or should we do a
        // shutdown(), then a blocking read(), then a close()?
        // http://web.deu.edu.tr/doc/soket/:
        // "Generally the difference between close() and shutdown() is: close()
        // closes the socket id for the process but the connection is still
        // opened if another process shares this socket id. The connection
        // stays opened both for read and write, and sometimes this is very
        // important. shutdown() breaks the connection for all processes sharing
        // the socket id. Those who try to read will detect EOF, and those who
        // try to write will receive SIGPIPE, possibly delayed while the kernel
        // socket buffer will be filled. Additionally, shutdown() has a second
        // argument which denotes how to close the connection: 0 means to
        // disable further reading, 1 to disable writing and 2 disables both."
        // https://linux.die.net/man/2/shutdown:
        // "The constants SHUT_RD, SHUT_WR, SHUT_RDWR have the value 0, 1, 2,
        // respectively, and are defined in <sys/socket.h>."
        s_session_shutdown = false;
        s_session_socket = session_socket;
        auto socket_cleanup = scope_guard::make_scope_guard([]() {
            // We can rely on close() to perform the equivalent of shutdown(SHUT_RDWR),
            // since we hold the only fd pointing to this socket.
            close(s_session_socket);
            // Assign an invalid fd to the socket variable so it can't be reused.
            s_session_socket = -1;
        });
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            throw_system_error("epoll_create1 failed");
        }
        auto epoll_cleanup = scope_guard::make_scope_guard([epoll_fd]() {
            close(epoll_fd);
        });
        int fds[] = {s_session_socket, s_server_shutdown_event_fd};
        for (size_t i = 0; i < array_size(fds); i++) {
            struct epoll_event ev;
            // We should only get EPOLLRDHUP from the client socket, but oh well.
            ev.events = EPOLLIN | EPOLLRDHUP;
            ev.data.fd = fds[i];
            if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[i], &ev)) {
                throw_system_error("epoll_ctl failed");
            }
        }
        struct epoll_event events[array_size(fds)];
        while (!s_session_shutdown) {
            // Block forever (we will be notified of shutdown).
            int ready_fd_count = epoll_wait(epoll_fd, events, array_size(events), -1);
            if (ready_fd_count == -1) {
                throw_system_error("epoll_wait failed");
            }
            session_event_t event = session_event_t::NOP;
            int* fds = nullptr;
            size_t fd_count = 0;
            for (int i = 0; i < ready_fd_count; i++) {
                const epoll_event ev = events[i];
                if (ev.data.fd == s_session_socket) {
                    // NB: Since many event flags are set in combination with others, the
                    // order we test them in matters! E.g., EPOLLIN seems to always be set
                    // whenever EPOLLRDHUP is set, so we need to test EPOLLRDHUP before
                    // testing EPOLLIN.
                    if (ev.events & EPOLLERR) {
                        // This flag is unmaskable, so we don't need to register for it.
                        int error = 0;
                        socklen_t err_len = sizeof(error);
                        // Ignore errors getting error message and default to generic error message.
                        getsockopt(s_session_socket, SOL_SOCKET, SO_ERROR, (void*)&error, &err_len);
                        throw_system_error("client socket error", error);
                    } else if (ev.events & EPOLLHUP) {
                        // This flag is unmaskable, so we don't need to register for it.
                        // Both ends of the socket have issued a shutdown(SHUT_WR) or equivalent.
                        retail_assert(!(ev.events & EPOLLERR));
                        event = session_event_t::CLIENT_SHUTDOWN;
                    } else if (ev.events & EPOLLRDHUP) {
                        // We must have already received a DISCONNECT message at this point
                        // and sent a reply followed by a FIN equivalent. The client must
                        // have received our reply and sent a FIN as well.
                        // REVIEW: Can we get both EPOLLHUP and EPOLLRDHUP when the client half-closes
                        // the socket after we half-close it?
                        retail_assert(!(ev.events & EPOLLHUP));
                        event = session_event_t::CLIENT_SHUTDOWN;
                    } else if (ev.events & EPOLLIN) {
                        retail_assert(!(ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)));
                        // Buffer used to send and receive all message data.
                        uint8_t msg_buf[MAX_MSG_SIZE] = {0};
                        // Buffer used to receive file descriptors.
                        int fd_buf[MAX_FD_COUNT] = {-1};
                        size_t fd_buf_size = array_size(fd_buf);
                        // Read client message with possible file descriptors.
                        size_t bytes_read = recv_msg_with_fds(s_session_socket, fd_buf, &fd_buf_size, msg_buf, sizeof(msg_buf));
                        // We shouldn't get EOF unless EPOLLRDHUP is set.
                        retail_assert(bytes_read > 0);
                        const message_t* msg = Getmessage_t(msg_buf);
                        const client_request_t* request = msg->msg_as_request();
                        event = request->event();
                        if (fd_buf_size > 0) {
                            fds = fd_buf;
                            fd_count = fd_buf_size;
                        }
                    }
                } else if (ev.data.fd == s_server_shutdown_event_fd) {
                    // We should always read the value 1 from a semaphore eventfd.
                    uint64_t val;
                    if (-1 == read(s_server_shutdown_event_fd, &val, sizeof(val))) {
                        throw_system_error("read(eventfd) failed");
                    }
                    retail_assert(val == 1);
                    event = session_event_t::SERVER_SHUTDOWN;
                } else {
                    // We don't monitor any other fds.
                    retail_assert(false);
                }
                retail_assert(event != session_event_t::NOP);
                apply_transition(event, fds, fd_count);
            }
        }
    }

    // Before this method is called, we have already received the log fd from the client
    // and mmapped it.
    // This method returns true for a commit decision and false for an abort decision.
    static bool tx_commit() {
        // At the process level, acquiring an advisory file lock in exclusive mode
        // guarantees there are no clients mapping the locator segment. It does not
        // guarantee there are no other threads in this process that have acquired
        // an exclusive lock, though (hence the additional mutex).
        if (-1 == flock(s_fd_offsets, LOCK_EX)) {
            throw_system_error("flock failed");
        }
        // Within our own process, we must have exclusive access to the locator segment.
        const std::lock_guard<std::mutex> lock(s_commit_lock);
        auto cleanup = scope_guard::make_scope_guard([]() {
            if (-1 == flock(s_fd_offsets, LOCK_UN)) {
                // Per C++11 semantics, throwing an exception from a destructor
                // will just call std::terminate(), no undefined behavior.
                throw_system_error("flock failed");
            }
            unmap_fd(s_log, sizeof(s_log));
        });

        std::set<int64_t> row_ids;

        for (auto i = 0; i < s_log->count; i++) {
            auto lr = s_log->log_records + i;

            if (row_ids.insert(lr->row_id).second) {
                if ((*s_shared_offsets)[lr->row_id] != lr->old_object) {
                    return false;
                }
            }
        }

        for (auto i = 0; i < s_log->count; i++) {
            auto lr = s_log->log_records + i;
            (*s_shared_offsets)[lr->row_id] = lr->new_object;
        }

        return true;
    }
};

}  // namespace db
}  // namespace gaia
