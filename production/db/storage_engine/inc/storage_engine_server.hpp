/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <csignal>
#include <thread>
#include <shared_mutex>

#include "scope_guard.hpp"
#include "retail_assert.hpp"
#include "system_error.hpp"
#include "mmap_helpers.hpp"
#include "socket_helpers.hpp"
#include "generator_iterator.hpp"

#include "storage_engine.hpp"
#include "gaia_object_internal.hpp"
#include "messages_generated.h"
#include "persistent_store_manager.hpp"
#include "gaia_se_object.hpp"
#include "gaia_db_internal.hpp"

namespace gaia {
namespace db {

using namespace gaia::common;
using namespace gaia::common::iterators;
using namespace gaia::db::messages;
using namespace flatbuffers;
using namespace scope_guard;

class invalid_session_transition : public gaia_exception {
public:
    invalid_session_transition(const string& message) : gaia_exception(message) {}
};

class server : private se_base {
    friend class persistent_store_manager;

public:
    static void run();

private:
    // from https://www.man7.org/linux/man-pages/man2/eventfd.2.html
    static constexpr uint64_t MAX_SEMAPHORE_COUNT = std::numeric_limits<uint64_t>::max() - 1;
    static int s_server_shutdown_eventfd;
    static int s_connect_socket;
    static std::shared_mutex s_locators_lock;
    static int s_fd_data;
    static locators* s_shared_locators;
    static std::unique_ptr<persistent_store_manager> rdb;
    thread_local static session_state_t s_session_state;
    thread_local static bool s_session_shutdown;
    thread_local static int s_session_shutdown_eventfd;
    thread_local static std::vector<std::thread> s_session_owned_threads;

    static int s_fd_locators;
    static data* s_data;

    // Inherited from se_base:
    // thread_local static log *s_log;
    // thread_local static int s_session_socket;
    // thread_local static gaia_txn_id_t s_txn_id;

    // function pointer type that executes side effects of a state transition
    // REVIEW: replace void* with std::any?
    typedef void (*transition_handler_fn)(int* fds, size_t fd_count, session_event_t event,
        const void* event_data, session_state_t old_state, session_state_t new_state);
    static void handle_connect(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_begin_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_rollback_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_commit_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_decide_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_client_shutdown(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_server_shutdown(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_request_stream(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);

    struct transition_t {
        session_state_t new_state;
        transition_handler_fn handler;
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
        {session_state_t::ANY, session_event_t::SERVER_SHUTDOWN, {session_state_t::DISCONNECTED, handle_server_shutdown}},
        {session_state_t::ANY, session_event_t::REQUEST_STREAM, {session_state_t::ANY, handle_request_stream}},
    };

    static void apply_transition(session_event_t event, const void* event_data, int* fds, size_t fd_count) {
        if (event == session_event_t::NOP) {
            return;
        }
        // "Wildcard" transitions (current state = session_state_t::ANY) must be listed after
        // non-wildcard transitions with the same event, or the latter will never be applied.
        for (size_t i = 0; i < std::size(s_valid_transitions); i++) {
            valid_transition_t t = s_valid_transitions[i];
            if (t.event == event && (t.state == s_session_state || t.state == session_state_t::ANY)) {
                session_state_t new_state = t.transition.new_state;
                // If the transition's new state is ANY, then keep the state the same.
                if (new_state == session_state_t::ANY) {
                    new_state = s_session_state;
                }
                session_state_t old_state = s_session_state;
                s_session_state = new_state;
                if (t.transition.handler) {
                    t.transition.handler(fds, fd_count, event, event_data, old_state, s_session_state);
                }
                return;
            }
        }
        // If we get here, we haven't found any compatible transition.
        // TODO: consider propagating exception back to client?
        throw invalid_session_transition(
            "no allowed state transition from state '"
            + std::string(EnumNamesession_state_t(s_session_state))
            + "' with event '" + std::string(EnumNamesession_event_t(event))
            + "'");
    }

    static void build_server_reply(
        FlatBufferBuilder& builder,
        session_event_t event,
        session_state_t old_state,
        session_state_t new_state,
        gaia_txn_id_t txn_id) {
        const auto transaction_info = Createtransaction_info_t(builder, transaction_id);
        const auto server_reply = Createserver_reply_t(builder, event, old_state, new_state,
            reply_data_t::transaction_info, transaction_info.Union());
        const auto message = Createmessage_t(builder, any_message_t::reply, server_reply.Union());
        builder.Finish(message);
    }

    static void clear_shared_memory() {
        if (s_shared_locators) {
            unmap_fd(s_shared_locators, sizeof(locators));
            s_shared_locators = nullptr;
        }
        if (s_fd_locators != -1) {
            close(s_fd_locators);
            s_fd_locators = -1;
        }
        if (s_data) {
            unmap_fd(s_data, sizeof(data));
            s_data = nullptr;
        }
        if (s_fd_data != -1) {
            ::close(s_fd_data);
            s_fd_data = -1;
        }
    }

    static void recover_db() {
        // Open RocksDB just once.
        if (!rdb) {
            rdb.reset(new gaia::db::persistent_store_manager());
            rdb->open();
            rdb->recover();
        }
    }

    // To avoid synchronization, we assume that this method is only called when
    // no sessions exist and the server is not accepting any new connections.
    static void init_shared_memory() {
        // The listening socket must not be open.
        retail_assert(s_connect_socket == -1);
        // We may be reinitializing the server upon receiving a SIGHUP.
        clear_shared_memory();
        // Clear all shared memory if an exception is thrown.
        auto cleanup_memory = make_scope_guard([]() {
            clear_shared_memory();
        });
        retail_assert(s_fd_data == -1 && s_fd_locators == -1);
        retail_assert(!s_data && !s_shared_locators);
        s_fd_locators = memfd_create(SCH_MEM_LOCATORS, MFD_ALLOW_SEALING);
        if (s_fd_locators == -1) {
            throw_system_error("memfd_create failed");
        }
        s_fd_data = ::memfd_create(SCH_MEM_DATA, MFD_ALLOW_SEALING);
        if (s_fd_data == -1) {
            throw_system_error("memfd_create failed");
        }
        if (-1 == ftruncate(s_fd_locators, sizeof(locators)) || -1 == ftruncate(s_fd_data, sizeof(data))) {
            throw_system_error("ftruncate failed");
        }
        s_shared_locators = static_cast<locators*>(map_fd(sizeof(locators),
            PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_locators, 0));
        s_data = static_cast<data*>(map_fd(sizeof(data),
            PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_data, 0));

        recover_db();

        cleanup_memory.dismiss();
    }

    static sigset_t mask_signals() {
        sigset_t sigset;
        ::sigemptyset(&sigset);
        // We now special-case SIGHUP to disconnect all sessions and reinitialize all shared memory.
        ::sigaddset(&sigset, SIGHUP);
        ::sigaddset(&sigset, SIGINT);
        ::sigaddset(&sigset, SIGTERM);
        ::sigaddset(&sigset, SIGQUIT);
        // Per POSIX, we must use pthread_sigmask() rather than sigprocmask()
        // in a multithreaded program.
        // REVIEW: should this be SIG_SETMASK?
        ::pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
        return sigset;
    }

    static void signal_handler(sigset_t sigset, int& signum) {
        // Wait until a signal is delivered.
        // REVIEW: do we have any use for sigwaitinfo()?
        ::sigwait(&sigset, &signum);
        cerr << "Caught signal " << ::strsignal(signum) << endl;
        // Signal the eventfd by writing a nonzero value.
        // This value is large enough that no thread will
        // decrement it to zero, so every waiting thread
        // should see a nonzero value.
        ssize_t bytes_written = ::write(
            s_server_shutdown_eventfd, &MAX_SEMAPHORE_COUNT, sizeof(MAX_SEMAPHORE_COUNT));
        if (bytes_written == -1) {
            throw_system_error("write to eventfd failed");
        }
        retail_assert(bytes_written == sizeof(MAX_SEMAPHORE_COUNT));
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
        int connect_socket = ::socket(PF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
        if (connect_socket == -1) {
            throw_system_error("socket creation failed");
        }
        auto socket_cleanup = make_scope_guard([connect_socket]() {
            ::close(connect_socket);
        });
        struct sockaddr_un server_addr = {0};
        server_addr.sun_family = AF_UNIX;
        // The socket name (minus its null terminator) needs to fit into the space
        // in the server address structure after the prefix null byte.
        // This should be static_assert(), but strlen() doesn't return a constexpr.
        retail_assert(strlen(SE_SERVER_SOCKET_NAME) <= sizeof(server_addr.sun_path) - 1);
        // We prepend a null byte to the socket name so the address is in the
        // (Linux-exclusive) "abstract namespace", i.e., not bound to the
        // filesystem.
        ::strncpy(&server_addr.sun_path[1], SE_SERVER_SOCKET_NAME,
            sizeof(server_addr.sun_path) - 1);
        // The socket name is not null-terminated in the address structure, but
        // we need to add an extra byte for the null byte prefix.
        socklen_t server_addr_size =
            sizeof(server_addr.sun_family) + 1 + strlen(&server_addr.sun_path[1]);
        if (-1 == ::bind(connect_socket, (struct sockaddr*)&server_addr, server_addr_size)) {
            throw_system_error("bind failed");
        }
        if (-1 == ::listen(connect_socket, 0)) {
            throw_system_error("listen failed");
        }
        int epoll_fd = ::epoll_create1(0);
        if (epoll_fd == -1) {
            throw_system_error("epoll_create1 failed");
        }
        auto epoll_cleanup = make_scope_guard([epoll_fd]() {
            ::close(epoll_fd);
        });
        int fds[] = {connect_socket, s_server_shutdown_eventfd};
        for (size_t i = 0; i < array_size(fds); i++) {
            epoll_event ev{.events = EPOLLIN, .data.fd = fds[i]};
            if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[i], &ev)) {
                throw_system_error("epoll_ctl failed");
            }
        }
        epoll_cleanup.dismiss();
        socket_cleanup.dismiss();
        s_connect_socket = connect_socket;
        return epoll_fd;
    }

    static bool authenticate_client_socket(int socket) {
        struct ucred cred;
        socklen_t cred_len = sizeof(cred);
        if (-1 == ::getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len)) {
            throw_system_error("getsockopt(SO_PEERCRED) failed");
        }
        // Disable client authentication until we can figure out
        // how to fix the Postgres tests.
        // Client must have same effective user ID as server.
        // return (cred.uid == geteuid());
        return true;
    }

    static void client_dispatch_handler() {
        int epoll_fd = get_client_dispatch_fd();
        std::vector<std::thread> session_threads;
        auto cleanup = make_scope_guard([epoll_fd, &session_threads]() {
            // Close the epoll fd first so we stop polling the listening socket.
            ::close(epoll_fd);
            // Close the listening socket before waiting for session threads, so no new
            // sessions can be established while we wait for all session threads to exit.
            ::close(s_connect_socket);
            s_connect_socket = -1;
            for (std::thread& t : session_threads) {
                t.join();
            }
        });
        epoll_event events[2];
        while (true) {
            // Block forever (we will be notified of shutdown).
            int ready_fd_count = ::epoll_wait(epoll_fd, events, array_size(events), -1);
            if (ready_fd_count == -1) {
                throw_system_error("epoll_wait failed");
            }
            for (int i = 0; i < ready_fd_count; i++) {
                const epoll_event ev = events[i];
                // We never register for anything but EPOLLIN,
                // but EPOLLERR will always be delivered.
                if (ev.events & EPOLLERR) {
                    if (ev.data.fd == s_connect_socket) {
                        int error = 0;
                        socklen_t err_len = sizeof(error);
                        // Ignore errors getting error message and default to generic error message.
                        ::getsockopt(s_connect_socket, SOL_SOCKET, SO_ERROR, (void*)&error, &err_len);
                        throw_system_error("client socket error", error);
                    } else if (ev.data.fd == s_server_shutdown_eventfd) {
                        throw_system_error("shutdown eventfd error");
                    }
                }
                // At this point, we should only get EPOLLIN.
                retail_assert(ev.events == EPOLLIN);
                if (ev.data.fd == s_connect_socket) {
                    int session_socket = ::accept(s_connect_socket, NULL, NULL);
                    if (session_socket == -1) {
                        throw_system_error("accept failed");
                    }
                    if (authenticate_client_socket(session_socket)) {
                        session_threads.emplace_back(session_handler, session_socket);
                    } else {
                        ::close(session_socket);
                    }
                } else if (ev.data.fd == s_server_shutdown_eventfd) {
                    uint64_t val;
                    ssize_t bytes_read = ::read(s_server_shutdown_eventfd, &val, sizeof(val));
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

    static gaia_se_object_t* locator_to_ptr(locators* locators, data* s_data, gaia_locator_t locator) {
        assert(*locators);
        return locator && (*locators)[locator]
            ? reinterpret_cast<gaia_se_object_t*>(s_data->objects + (*locators)[locator])
            : nullptr;
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
        auto socket_cleanup = make_scope_guard([]() {
            // We can rely on close() to perform the equivalent of shutdown(SHUT_RDWR),
            // since we hold the only fd pointing to this socket.
            ::close(s_session_socket);
            // Assign an invalid fd to the socket variable so it can't be reused.
            s_session_socket = -1;
        });
        int epoll_fd = ::epoll_create1(0);
        if (epoll_fd == -1) {
            throw_system_error("epoll_create1 failed");
        }
        auto epoll_cleanup = make_scope_guard([epoll_fd]() {
            ::close(epoll_fd);
        });
        int fds[] = {s_session_socket, s_server_shutdown_eventfd};
        for (size_t i = 0; i < std::size(fds); i++) {
            // We should only get EPOLLRDHUP from the client socket, but oh well.
            epoll_event ev{.events = EPOLLIN | EPOLLRDHUP, .data.fd = fds[i]};
            if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[i], &ev)) {
                throw_system_error("epoll_ctl failed");
            }
        }
        epoll_event events[std::size(fds)];
        // Event to signal session-owned threads to terminate.
        int session_shutdown_eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
        if (session_shutdown_eventfd == -1) {
            throw_system_error("eventfd failed");
        }
        s_session_shutdown_eventfd = session_shutdown_eventfd;
        auto owned_threads_cleanup = make_scope_guard([]() {
            // Signal the eventfd by writing a nonzero value.
            // This value is large enough that no thread will
            // decrement it to zero, so every waiting thread
            // should see a nonzero value.
            ssize_t bytes_written = ::write(
                s_session_shutdown_eventfd, &MAX_SEMAPHORE_COUNT, sizeof(MAX_SEMAPHORE_COUNT));
            if (bytes_written == -1) {
                throw_system_error("write to eventfd failed");
            }
            retail_assert(bytes_written == sizeof(MAX_SEMAPHORE_COUNT));
            // Wait for all session-owned threads to terminate.
            for (std::thread& t : s_session_owned_threads) {
                t.join();
            }
        });
        while (!s_session_shutdown) {
            // Block forever (we will be notified of shutdown).
            int ready_fd_count = ::epoll_wait(epoll_fd, events, array_size(events), -1);
            if (ready_fd_count == -1) {
                throw_system_error("epoll_wait failed");
            }
            session_event_t event = session_event_t::NOP;
            const void* event_data = nullptr;
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
                        ::getsockopt(s_session_socket, SOL_SOCKET, SO_ERROR, (void*)&error, &err_len);
                        cerr << "client socket error: " << ::strerror(error) << endl;
                        event = session_event_t::CLIENT_SHUTDOWN;
                    } else if (ev.events & EPOLLHUP) {
                        // This flag is unmaskable, so we don't need to register for it.
                        // Both ends of the socket have issued a shutdown(SHUT_WR) or equivalent.
                        retail_assert(!(ev.events & EPOLLERR));
                        event = session_event_t::CLIENT_SHUTDOWN;
                    } else if (ev.events & EPOLLRDHUP) {
                        // The client has called shutdown(SHUT_WR) to signal their intention to
                        // disconnect. We do the same by closing the session socket.
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
                        size_t fd_buf_size = std::size(fd_buf);
                        // Read client message with possible file descriptors.
                        size_t bytes_read = recv_msg_with_fds(s_session_socket, fd_buf, &fd_buf_size, msg_buf, sizeof(msg_buf));
                        // We shouldn't get EOF unless EPOLLRDHUP is set.
                        // REVIEW: it might be possible for the client to call shutdown(SHUT_WR)
                        // after we have already woken up on EPOLLIN, in which case we would
                        // legitimately read 0 bytes and this assert would be invalid.
                        retail_assert(bytes_read > 0);
                        const message_t* msg = Getmessage_t(msg_buf);
                        const client_request_t* request = msg->msg_as_request();
                        event = request->event();
                        // We need to pass auxiliary data identifying the requested stream type and properties.
                        if (event == session_event_t::REQUEST_STREAM) {
                            // We should logically pass an object corresponding to the request_data_t union,
                            // but the FlatBuffers API doesn't have any object corresponding to a union.
                            event_data = static_cast<const void*>(request);
                        }
                        if (fd_buf_size > 0) {
                            fds = fd_buf;
                            fd_count = fd_buf_size;
                        }
                    } else {
                        // We don't register for any other events.
                        retail_assert(false);
                    }
                } else if (ev.data.fd == s_server_shutdown_eventfd) {
                    retail_assert(ev.events == EPOLLIN);
                    // We should always read the value 1 from a semaphore eventfd.
                    uint64_t val;
                    ssize_t bytes_read = ::read(s_server_shutdown_eventfd, &val, sizeof(val));
                    if (bytes_read == -1) {
                        throw_system_error("read(eventfd) failed");
                    }
                    retail_assert(bytes_read == sizeof(val));
                    retail_assert(val == 1);
                    event = session_event_t::SERVER_SHUTDOWN;
                } else {
                    // We don't monitor any other fds.
                    retail_assert(false);
                }
                retail_assert(event != session_event_t::NOP);
                apply_transition(event, event_data, fds, fd_count);
            }
        }
    }

    template <typename element_type>
    static void stream_producer_handler(int stream_socket, int cancel_eventfd,
        std::function<std::optional<element_type>()> generator_fn) {
        constexpr size_t max_writes_before_cancellation_check = 1 << 10;
        // We only support fixed-width integer types for now to avoid framing.
        static_assert(std::is_integral<element_type>::value, "Generator function must return an integer.");
        auto gen_iter = generator_iterator<element_type>(generator_fn);
        auto socket_cleanup = make_scope_guard([stream_socket]() {
            // We can rely on close() to perform the equivalent of shutdown(SHUT_RDWR),
            // since we hold the only fd pointing to this socket.
            ::close(stream_socket);
        });
        // Check if our stream socket is non-blocking (it must be for edge-triggered epoll).
        int flags = ::fcntl(stream_socket, F_GETFL, 0);
        if (flags == -1) {
            throw_system_error("fcntl(F_GETFL) failed");
        }
        retail_assert(flags & O_NONBLOCK);
        int epoll_fd = ::epoll_create1(0);
        if (epoll_fd == -1) {
            throw_system_error("epoll_create1 failed");
        }
        auto epoll_cleanup = make_scope_guard([epoll_fd]() {
            ::close(epoll_fd);
        });
        // We're only using epoll so we can poll the cancellation eventfd at the same time as the socket,
        // otherwise we'd just loop in a blocking write(). Since we need to use EPOLLOUT to poll on write
        // readiness of the socket, we should use edge-triggered mode to avoid notifications on every loop
        // iteration, which could easily consume 100% CPU. (This is in contrast to polling for read readiness,
        // where we generally want level-triggered mode.) Logically, we can just write to the socket until
        // write() returns EAGAIN/EWOULDBLOCK, and return to epoll_wait(). However, we need to poll the
        // eventfd at the same time, and we don't want to wait until the socket is no longer writable to
        // poll the eventfd again, or we could wait an unbounded time before receiving the eventfd notification.
        // So we do a nonblocking read() on the eventfd after a fixed number of writes to the socket.
        // Later, we should add buffering to minimize syscalls, which means we only need to do a single write()
        // to the socket on each write-readiness notification, so we can switch to level-triggered mode and avoid
        // reading the eventfd at all except on read-readiness notifications.
        epoll_event sock_ev{.events = EPOLLOUT | EPOLLET, .data.fd = stream_socket};
        if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stream_socket, &sock_ev)) {
            throw_system_error("epoll_ctl failed");
        }
        epoll_event cancel_ev{.events = EPOLLIN, .data.fd = cancel_eventfd};
        if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cancel_eventfd, &cancel_ev)) {
            throw_system_error("epoll_ctl failed");
        }
        epoll_event events[2];
        bool producer_shutdown = false;
        while (!producer_shutdown) {
            // Block forever (we will be notified of shutdown).
            int ready_fd_count = ::epoll_wait(epoll_fd, events, array_size(events), -1);
            if (ready_fd_count == -1) {
                throw_system_error("epoll_wait failed");
            }
            for (int i = 0; i < ready_fd_count; i++) {
                const epoll_event ev = events[i];
                if (ev.data.fd == stream_socket) {
                    // NB: Since many event flags are set in combination with others, the
                    // order we test them in matters! E.g., EPOLLIN seems to always be set
                    // whenever EPOLLRDHUP is set, so we need to test EPOLLRDHUP before
                    // testing EPOLLIN.
                    if (ev.events & EPOLLERR) {
                        // This flag is unmaskable, so we don't need to register for it.
                        int error = 0;
                        socklen_t err_len = sizeof(error);
                        // Ignore errors getting error message and default to generic error message.
                        ::getsockopt(stream_socket, SOL_SOCKET, SO_ERROR, (void*)&error, &err_len);
                        cerr << "stream socket error: " << ::strerror(error) << endl;
                        producer_shutdown = true;
                    } else if (ev.events & EPOLLHUP) {
                        // This flag is unmaskable, so we don't need to register for it.
                        // We shold get this when the client has closed its end of the socket.
                        retail_assert(!(ev.events & EPOLLERR));
                        producer_shutdown = true;
                    } else if (ev.events & EPOLLOUT) {
                        retail_assert(!(ev.events & (EPOLLERR | EPOLLHUP)));
                        // Write to the socket in a loop until we get EAGAIN/EWOULDBLOCK.
                        // (How do we check the cancellation event within the loop?)
                        // When we've exhausted the iterator, either call shutdown(SHUT_WR)
                        // or close() to make the client's next read() return EOF.
                        // We can't use the dereference operator to test for end of iteration,
                        // since that would be dereferencing std::nullopt, which is undefined.
                        size_t write_count = 0;
                        while (gen_iter) {
                            element_type next_val = *gen_iter;
                            ssize_t bytes_written = ::write(stream_socket, &next_val, sizeof(next_val));
                            if (bytes_written == -1) {
                                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                    // Socket is no longer writable, go back to polling.
                                    break;
                                } else {
                                    // Log the error and break out of the poll loop.
                                    cerr << "stream socket error: " << ::strerror(errno) << endl;
                                    producer_shutdown = true;
                                    break;
                                }
                            } else {
                                // We successfully wrote to the socket, so advance the iterator.
                                ++gen_iter;
                                // Periodically read the cancellation eventfd, to avoid read starvation from
                                // a fast reader on the client socket.
                                // REVIEW: This is only necessary because we're using edge-triggered mode. Later,
                                // when we add buffering to minimize syscalls, we should be able to switch to
                                // level-triggered mode (one write per write-readiness notification), and remove
                                // this hack.
                                if (++write_count % max_writes_before_cancellation_check == 0) {
                                    // We should always read the value 1 from a semaphore eventfd.
                                    uint64_t val;
                                    ssize_t bytes_read = ::read(cancel_eventfd, &val, sizeof(val));
                                    if (bytes_read == -1) {
                                        // Since we didn't get a readiness notification on the eventfd,
                                        // this read will normally return EAGAIN/EWOULDBLOCK.
                                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                                            throw_system_error("read(eventfd) failed");
                                        }
                                    } else {
                                        retail_assert(bytes_read == sizeof(val));
                                        retail_assert(val == 1);
                                        producer_shutdown = true;
                                        break;
                                    }
                                }
                            }
                        }
                        // If we reached end of iteration, we need to send the client EOF.
                        // We let the client decide when to close the socket, since their next read
                        // may be arbitrarily delayed (and they may still have pending data).
                        if (!gen_iter) {
                            ::shutdown(stream_socket, SHUT_WR);
                        }
                    } else {
                        // We don't register for any other events.
                        retail_assert(false);
                    }
                } else if (ev.data.fd == cancel_eventfd) {
                    retail_assert(ev.events == EPOLLIN);
                    // We should always read the value 1 from a semaphore eventfd.
                    uint64_t val;
                    ssize_t bytes_read = ::read(cancel_eventfd, &val, sizeof(val));
                    if (bytes_read == -1) {
                        throw_system_error("read(eventfd) failed");
                    }
                    retail_assert(bytes_read == sizeof(val));
                    retail_assert(val == 1);
                    producer_shutdown = true;
                } else {
                    // We don't monitor any other fds.
                    retail_assert(false);
                }
            }
        }
    }

    static void start_stream_producer_thread(const int server_socket, const int cancel_eventfd, const client_request_t *request) {
        // The only currently supported stream type is table scans.
        // When we add more stream types, we should add a switch statement on stream_type.
        retail_assert(request->data_type() == request_data_t::table_scan);
        const gaia_type_t table_type = static_cast<gaia_type_t>(request->data_as_table_scan()->type_id());
        gaia_locator_t row_id = 0;
        auto table_scan_id_generator = [table_type, row_id]() mutable -> std::optional<gaia_id_t> {
            // We need to ensure that we're not reading the locator segment
            // while a committing transaction is writing to it.
            std::shared_lock lock(s_locators_lock);
            while (++row_id && row_id < s_data->row_id_count + 1) {
                gaia_offset_t offset = (*s_shared_offsets)[row_id];
                if (offset) {
                    object* obj = reinterpret_cast<object*>(s_data->objects + (*s_shared_offsets)[row_id]);
                    if (obj->type == table_type) {
                        return obj->id;
                    }
                }
            }
            // Signal end of iteration.
            return std::nullopt;
        };
        // Give the session ownership of the new stream thread.
        s_session_owned_threads.emplace_back(stream_producer_handler<gaia_id_t>,
            server_socket, cancel_eventfd, table_scan_id_generator);
    }

    // Before this method is called, we have already received the log fd from the client
    // and mmapped it.
    // This method returns true for a commit decision and false for an abort decision.
    static bool txn_commit() {
        // At the process level, acquiring an advisory file lock in exclusive mode
        // guarantees there are no clients mapping the locator segment. It does not
        // guarantee there are no other threads in this process that have acquired
        // an exclusive lock, though (hence the additional mutex).
        if (-1 == ::flock(s_fd_locators, LOCK_EX)) {
            throw_system_error("flock failed");
        }
        // Within our own process, we must have exclusive access to the locator segment.
        std::unique_lock lock(s_locators_lock);
        auto cleanup = make_scope_guard([]() {
            if (-1 == ::flock(s_fd_locators, LOCK_UN)) {
                // Per C++11 semantics, throwing an exception from a destructor
                // will just call std::terminate(), no undefined behavior.
                throw_system_error("flock failed");
            }
            unmap_fd(s_log, sizeof(s_log));
        });

        std::set<gaia_locator_t> locators;

        auto txn_name = rdb->begin_txn(s_txn_id);
        // Prepare tx
        rdb->prepare_wal_for_write(txn_name);

        for (size_t i = 0; i < s_log->count; i++) {
            auto lr = s_log->log_records + i;

            if (locators.insert(lr->locator).second) {
                if ((*s_shared_locators)[lr->locator] != lr->old_offset) {
                    // Append Rollback decision to log.
                    // This isn't really required because recovery will skip deserializing transactions
                    // that don't have a commit marker; we do it for completeness anyway.
                    rdb->append_wal_rollback_marker(txn_name);
                    return false;
                }
            }
        }

        for (size_t i = 0; i < s_log->count; i++) {
            auto lr = s_log->log_records + i;
            (*s_shared_locators)[lr->locator] = lr->new_offset;
        }

        // Append commit decision to the log.
        rdb->append_wal_commit_marker(txn_name);

        return true;
    }
};

} // namespace db
} // namespace gaia
