/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// C++ system headers
#include <csignal>
#include <thread>
// C system headers
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <libexplain/mmap.h>
#include <libexplain/munmap.h>
// internal headers
#include "storage_engine.hpp"
#include "array_size.hpp"
#include "runtime_error.hpp"
#include "socket_helpers.hpp"
#include "messages_generated.h"

namespace gaia
{
namespace db
{
    using namespace common;
    using namespace messages;
    using namespace flatbuffers;

    class gaia_server: private gaia_mem_base
    {
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
        thread_local static SessionState s_session_state;
        thread_local static bool s_session_shutdown;

        // inherited from gaia_mem_base:
        // static int s_fd_offsets;
        // static data *s_data;
        // thread_local static log *s_log;
        // thread_local static int s_session_socket;

        // function pointer type that executes side effects of a state transition
        typedef void (*TransitionHandler)(int* fds, size_t fd_count, SessionEvent event, SessionState old_state, SessionState new_state);
        static void handle_connect(int*, size_t, SessionEvent, SessionState, SessionState);
        static void handle_begin_txn(int*, size_t, SessionEvent, SessionState, SessionState);
        static void handle_abort_txn(int*, size_t, SessionEvent, SessionState, SessionState);
        static void handle_commit_txn(int*, size_t, SessionEvent, SessionState, SessionState);
        static void handle_decide_txn(int*, size_t, SessionEvent, SessionState, SessionState);
        static void handle_client_shutdown(int*, size_t, SessionEvent, SessionState, SessionState);
        static void handle_server_shutdown(int*, size_t, SessionEvent, SessionState, SessionState);

        struct Transition {
            SessionState new_state;
            TransitionHandler handler;
        };

        struct ValidTransition {
            SessionState state;
            SessionEvent event;
            Transition transition;
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
        // -> CONNECTED (client aborted transaction)
        // TXN_COMMITTING (server decided to commit or abort transaction)
        // -> CONNECTED

        static constexpr ValidTransition s_valid_transitions[] = {
            { SessionState::DISCONNECTED, SessionEvent::CONNECT, { SessionState::CONNECTED, handle_connect } },
            { SessionState::CONNECTED, SessionEvent::BEGIN_TXN, { SessionState::TXN_IN_PROGRESS, handle_begin_txn } },
            { SessionState::TXN_IN_PROGRESS, SessionEvent::ABORT_TXN, { SessionState::CONNECTED, handle_abort_txn } },
            { SessionState::TXN_IN_PROGRESS, SessionEvent::COMMIT_TXN, { SessionState::TXN_COMMITTING, handle_commit_txn } },
            { SessionState::TXN_COMMITTING, SessionEvent::DECIDE_TXN_COMMIT, { SessionState::CONNECTED, handle_decide_txn } },
            { SessionState::TXN_COMMITTING, SessionEvent::DECIDE_TXN_ABORT, { SessionState::CONNECTED, handle_decide_txn } },
            { SessionState::CONNECTED, SessionEvent::CLIENT_SHUTDOWN, { SessionState::DISCONNECTED, handle_client_shutdown } },
            { SessionState::CONNECTED, SessionEvent::SERVER_SHUTDOWN, { SessionState::DISCONNECTING, handle_server_shutdown } },
            { SessionState::DISCONNECTING, SessionEvent::CLIENT_SHUTDOWN, { SessionState::DISCONNECTED, handle_client_shutdown } },
        };

        static void apply_transition(SessionEvent event, int* fds, size_t fd_count) {
            if (event == SessionEvent::NOP) {
                return;
            }
            for (size_t i = 0; i < array_size(s_valid_transitions); i++) {
                ValidTransition t = s_valid_transitions[i];
                if (t.state == s_session_state && t.event == event) {
                    SessionState old_state = s_session_state;
                    s_session_state = t.transition.new_state;
                    if (t.transition.handler) {
                        t.transition.handler(fds, fd_count, event, old_state, s_session_state);
                    }
                    break;
                } else {
                    // consider propagating exception back to client?
                    throw std::runtime_error(
                        "no allowed state transition from state '" +
                        std::string(EnumNameSessionState(s_session_state)) +
                        "' with event '" +
                        std::string(EnumNameSessionEvent(event)) + "'");
                }
            }
        }

        static void build_server_reply(FlatBufferBuilder& builder, SessionEvent event, SessionState old_state, SessionState new_state) {
            auto server_reply = CreateServerReply(builder, event, old_state, new_state);
            CreateMessage(builder, AnyMessage::reply, server_reply.Union());
        }

        static void init_shared_memory()
        {
            assert(!s_fd_data && !s_fd_offsets);
            s_fd_offsets = memfd_create(SCH_MEM_OFFSETS, MFD_ALLOW_SEALING);
            if (s_fd_offsets == -1)
            {
                throw_runtime_error("memfd_create failed");
            }
            s_fd_data = memfd_create(SCH_MEM_DATA, MFD_ALLOW_SEALING);
            if (s_fd_data == -1)
            {
                throw_runtime_error("memfd_create failed");
            }
            if (-1 == ftruncate(s_fd_offsets, sizeof(offsets))
                || -1 == ftruncate(s_fd_data, sizeof(data)))
            {
                throw_runtime_error("ftruncate failed");
            }
            s_shared_offsets = (offsets*)mmap(nullptr, sizeof(offsets),
                PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_offsets, 0);
            if (s_shared_offsets == MAP_FAILED)
            {
                const char* reason = explain_mmap(nullptr, sizeof(offsets),
                    PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_offsets, 0);
                throw std::runtime_error(reason);
            }
            s_data = (data*)mmap(nullptr, sizeof(data),
                PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_data, 0);
            if (s_data == MAP_FAILED)
            {
                const char* reason = explain_mmap(nullptr, sizeof(data),
                    PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_data, 0);
                throw std::runtime_error(reason);
            }
        }

        static sigset_t mask_signals()
        {
            sigset_t sigset;
            sigemptyset(&sigset);
            // XXX: do we want to handle SIGHUP differently at some point (e.g. reload config)?
            sigaddset(&sigset, SIGHUP);
            sigaddset(&sigset, SIGINT);
            sigaddset(&sigset, SIGTERM);
            sigaddset(&sigset, SIGQUIT);
            // Per POSIX, we must use pthread_sigmask() rather than sigprocmask()
            // in a multithreaded program.
            // XXX: should this be SIG_SETMASK?
            pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
            return sigset;
        }

        static void signal_handler(sigset_t sigset, int& signum)
        {
            // Wait until a signal is delivered.
            // XXX: do we have any use for sigwaitinfo()?
            sigwait(&sigset, &signum);
            // Signal the eventfd by writing a nonzero value.
            // This value is large enough that no thread will
            // decrement it to zero, so every waiting thread
            // should see a nonzero value.
            ssize_t bytes_written = write(
                s_server_shutdown_event_fd, &MAX_SEMAPHORE_COUNT, sizeof(MAX_SEMAPHORE_COUNT));
            assert(bytes_written == sizeof(MAX_SEMAPHORE_COUNT) || bytes_written == -1);
            if (bytes_written == -1) {
                throw_runtime_error("write to eventfd failed");
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
                throw_runtime_error("socket creation failed");
            }
            struct sockaddr_un server_addr = {0};
            server_addr.sun_family = AF_UNIX;
            // The socket name (minus its null terminator) needs to fit into the space
            // in the server address structure after the prefix null byte.
            assert(sizeof(SERVER_CONNECT_SOCKET_NAME) - 1 <= sizeof(server_addr.sun_path) - 1);
            // We prepend a null byte to the socket name so the address is in the
            // (Linux-exclusive) "abstract namespace", i.e., not bound to the
            // filesystem.
            strncpy(&server_addr.sun_path[1], SERVER_CONNECT_SOCKET_NAME,
                    sizeof(server_addr.sun_path) - 1);
            // The socket name is not null-terminated in the address structure, but
            // we need to add an extra byte for the null byte prefix.
            socklen_t server_addr_size =
                sizeof(server_addr.sun_family) + 1 + strlen(&server_addr.sun_path[1]);
            if (-1 == ::bind(connect_socket, (struct sockaddr *)&server_addr,
                        server_addr_size)) {
                throw_runtime_error("bind failed");
            }
            if (-1 == listen(connect_socket, 0)) {
                throw_runtime_error("listen failed");
            }
            int epoll_fd = epoll_create1(0);
            if (epoll_fd == -1) {
                 throw_runtime_error("epoll_create1 failed");
            }
            int fds[] = {connect_socket, s_server_shutdown_event_fd};
            for (size_t i = 0; i < array_size(fds); i++) {
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = fds[i];
                if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[i], &ev)) {
                    throw_runtime_error("epoll_ctl failed");
                }
            }
            s_connect_socket = connect_socket;
            return epoll_fd;
        }

        static void client_dispatch_handler()
        {
            int epoll_fd = get_client_dispatch_fd();
            std::vector<std::thread> client_threads;
            struct epoll_event events[2];
            while (true) {
                // Block forever (we will be notified of shutdown).
                int ready_fd_count = epoll_wait(epoll_fd, events, array_size(events), -1);
                if (ready_fd_count == -1) {
                    throw_runtime_error("epoll_wait failed");
                }
                for (int i = 0; i < ready_fd_count; i++) {
                    const epoll_event ev = events[i];
                    // We never register for anything but EPOLLIN.
                    assert(ev.events == EPOLLIN);
                    if (ev.data.fd == s_connect_socket) {
                        int client_socket = accept(s_connect_socket, NULL, NULL);
                        if (client_socket == -1) {
                            throw_runtime_error("accept failed");
                        }
                        client_threads.emplace_back(client_thread, client_socket);
                    } else if (ev.data.fd == s_server_shutdown_event_fd) {
                        // We should always read the value 1 from a semaphore eventfd.
                        uint64_t val;
                        assert(1 == read(s_server_shutdown_event_fd, &val, sizeof(val)));
                        goto shutdown;
                    } else {
                        // We don't monitor any other fds.
                        assert(false);
                    }
                }
            }
        shutdown:
            close(epoll_fd);
            for (std::thread &t : client_threads) {
                t.join();
            }
        }

        static void client_thread(int client_socket)
        {
            // XXX: how do we gracefully close the client socket?
            // do we need to issue a nonblocking read() first?
            // then do we need to call shutdown() before close()?
            // with TCP shutdown() will send a FIN packet, but what
            // does it do for a Unix socket? or should we do a
            // shutdown(), then a blocking read(), then a close()?
            // http://web.deu.edu.tr/doc/soket/:
            // "Generally the difference between close() and shutdown() is: close()
            // closes the socket id for the process but the connection is still
            // opened if another process shares this socket id. The connection
            // stays opened both for read and write, and sometimes this is very
            // important. shutdown() breaks the connection for all processes sharing
            // the socket id. Those who try to read will detect EOF, and those who
            // try to write will reseive SIGPIPE, possibly delayed while the kernel
            // socket buffer will be filled. Additionally, shutdown() has a second
            // argument which denotes how to close the connection: 0 means to
            // disable further reading, 1 to disable writing and 2 disables both.""
            // https://linux.die.net/man/2/shutdown:
            // "The constants SHUT_RD, SHUT_WR, SHUT_RDWR have the value 0, 1, 2,
            // respectively, and are defined in <sys/socket.h>."
            s_session_shutdown = false;
            s_session_socket = client_socket;
            // We assume that the client thread is always launched in response to
            // a client connection request.
            s_session_state = SessionState::CONNECTING;
            int epoll_fd = epoll_create1(0);
            if (epoll_fd == -1) {
                 throw_runtime_error("epoll_create1 failed");
            }
            int fds[] = {s_session_socket, s_server_shutdown_event_fd};
            for (size_t i = 0; i < array_size(fds); i++) {
                struct epoll_event ev;
                // We should only get EPOLLRDHUP from the client socket, but oh well.
                ev.events = EPOLLIN | EPOLLRDHUP;
                ev.data.fd = fds[i];
                if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[i], &ev)) {
                    throw_runtime_error("epoll_ctl failed");
                }
            }
            struct epoll_event events[array_size(fds)];
            while (!s_session_shutdown) {
                // Block forever (we will be notified of shutdown).
                int ready_fd_count = epoll_wait(epoll_fd, events, array_size(events), -1);
                if (ready_fd_count == -1) {
                    throw_runtime_error("epoll_wait failed");
                }
                SessionEvent event = SessionEvent::NOP;
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
                            getsockopt(s_session_socket, SOL_SOCKET, SO_ERROR, (void *)&error, &err_len);
                            throw_runtime_error("client socket error", error);
                        } else if (ev.events & EPOLLHUP) {
                            // This flag is unmaskable, so we don't need to register for it.
                            // Both ends of the socket have issued a shutdown(SHUT_WR) or equivalent.
                            assert(!(ev.events & EPOLLRDHUP));
                            event = SessionEvent::CLIENT_SHUTDOWN;
                        } else if (ev.events & EPOLLRDHUP) {
                            // We must have already received a DISCONNECT message at this point
                            // and sent a reply followed by a FIN equivalent. The client must
                            // have received our reply and sent a FIN as well.
                            // Can we get both EPOLLHUP and EPOLLRDHUP when the client half-closes
                            // the socket after we half-close it?
                            assert(!(ev.events & EPOLLHUP));
                            event = SessionEvent::CLIENT_SHUTDOWN;
                        } else if (ev.events & EPOLLIN) {
                            assert(!(ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)));
                            // Buffer used to send and receive all message data.
                            uint8_t msg_buf[MAX_MSG_SIZE] = {0};
                            // Buffer used to receive file descriptors.
                            int fd_buf[MAX_FD_COUNT] = {-1};
                            size_t fd_buf_size = array_size(fd_buf);
                            // Read client message with possible file descriptors.
                            size_t bytes_read = recv_msg_with_fds(client_socket, fd_buf, &fd_buf_size, msg_buf, sizeof(msg_buf));
                            // We shouldn't get EOF unless EPOLLRDHUP is set.
                            assert(bytes_read > 0);
                            const Message *msg = GetMessage(msg_buf);
                            const ClientRequest *req = msg->msg_as_request();
                            event = req->event();
                            if (fd_buf_size > 0) {
                                fds = fd_buf;
                                fd_count = fd_buf_size;
                            }
                        }
                    } else if (ev.data.fd == s_server_shutdown_event_fd) {
                        // We should always read the value 1 from a semaphore eventfd.
                        uint64_t val;
                        if (-1 == read(s_server_shutdown_event_fd, &val, sizeof(val))) {
                            throw_runtime_error("read(eventfd) failed");
                        }
                        assert(val == 1);
                        event = SessionEvent::SERVER_SHUTDOWN;
                    } else {
                        // We don't monitor any other fds.
                        assert(false);
                    }
                    assert(event != SessionEvent::NOP);
                    apply_transition(event, fds, fd_count);
                }
            }
            close(epoll_fd);
            // We can rely on close() to perform the equivalent of shutdown(SHUT_RDWR),
            // since we hold the only fd pointing to this socket.
            close(s_session_socket);
            // Assign an invalid fd to the socket variable so it can't be reused.
            s_session_socket = -1;
        }

        // Before this method is called, we have already received the log fd from the client
        // and mmapped it.
        // This method returns true for a commit decision and false for an abort decision.
        static bool tx_commit()
        {
            // At the process level, acquiring an advisory file lock in exclusive mode
            // guarantees there are no clients mapping the locator segment. It does not
            // guarantee there are no other threads in this process that have acquired
            // an exclusive lock, though (hence the additional mutex).
            if (-1 == flock(s_fd_offsets, LOCK_EX))
            {
                throw_runtime_error("flock failed");
            }
            // Within our own process, we must have exclusive access to the locator segment.
            const std::lock_guard<std::mutex> lock(s_commit_lock);
            scope_guard::make_scope_guard([]() {
                if (-1 == flock(s_fd_offsets, LOCK_UN))
                {
                    // Per C++11 semantics, throwing an exception from a destructor
                    // will just call std::terminate(), no undefined behavior.
                    throw_runtime_error("flock failed");
                }
                if (-1 == munmap(s_log, sizeof(s_log))) {
                    const char* reason = explain_munmap(s_log, sizeof(s_log));
                    throw std::runtime_error(reason);
                }
            });

            std::set<int64_t> row_ids;

            for (auto i = 0; i < s_log->count; i++)
            {
                auto lr = s_log->log_records + i;

                if (row_ids.insert(lr->row_id).second)
                {
                    if ((*s_shared_offsets)[lr->row_id] != lr->old_object)
                    {
                        return false;
                    }
                }
            }

            for (auto i = 0; i < s_log->count; i++)
            {
                auto lr = s_log->log_records + i;
                (*s_shared_offsets)[lr->row_id] = lr->new_object;
            }

            return true;
        }
    };

} // db
} // gaia
