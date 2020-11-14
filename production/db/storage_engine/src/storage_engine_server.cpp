/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine_server.hpp"

#include "fd_helpers.hpp"
#include "persistent_store_manager.hpp"

using namespace std;

using namespace gaia::db;
using namespace gaia::db::messages;

void server::handle_connect(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::CONNECT, "Unexpected event received!");
    // This message should only be received after the client thread was first initialized.
    retail_assert(
        old_state == session_state_t::DISCONNECTED && new_state == session_state_t::CONNECTED,
        "Current event is inconsistent with state transition!");
    // We need to reply to the client with the fds for the data/locator segments.
    FlatBufferBuilder builder;
    build_server_reply(builder, session_event_t::CONNECT, old_state, new_state, s_txn_id);
    int send_fds[] = {s_fd_data, s_fd_locators};
    send_msg_with_fds(s_session_socket, send_fds, std::size(send_fds), builder.GetBufferPointer(), builder.GetSize());
}

void server::handle_begin_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::BEGIN_TXN, "Unexpected event received!");
    // This message should only be received while a transaction is in progress.
    retail_assert(
        old_state == session_state_t::CONNECTED && new_state == session_state_t::TXN_IN_PROGRESS,
        "Current event is inconsistent with state transition!");
    // Currently we don't need to alter any server-side state for opening a transaction.
    FlatBufferBuilder builder;
    s_txn_id = allocate_txn_id(s_data);
    build_server_reply(builder, session_event_t::CONNECT, old_state, new_state, s_txn_id);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());
}

void server::handle_rollback_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::ROLLBACK_TXN, "Unexpected event received!");
    // This message should only be received while a transaction is in progress.
    retail_assert(
        old_state == session_state_t::TXN_IN_PROGRESS && new_state == session_state_t::CONNECTED,
        "Current event is inconsistent with state transition!");
    // We just need to reset the session thread's txn_id without replying to the client.
    s_txn_id = 0;
}

void server::handle_commit_txn(
    int* fds, size_t fd_count, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::COMMIT_TXN, "Unexpected event received!");
    // This message should only be received while a transaction is in progress.
    retail_assert(
        old_state == session_state_t::TXN_IN_PROGRESS && new_state == session_state_t::TXN_COMMITTING,
        "Current event is inconsistent with state transition!");
    // Get the log fd and mmap it.
    retail_assert(fds && fd_count == 1, "Invalid fd data!");
    int fd_log = *fds;
    // Close our log fd on exit so the shared memory will be released when the client closes it.
    auto cleanup_fd = make_scope_guard([&]() {
        close_fd(fd_log);
    });
    // Check that the log memfd was sealed for writes.
    int seals = ::fcntl(fd_log, F_GET_SEALS);
    if (seals == -1)
    {
        throw_system_error("fcntl(F_GET_SEALS) failed");
    }
    retail_assert(seals & F_SEAL_WRITE, "Log fd was not sealed for write!");
    // Linux won't let us create a shared read-only mapping if F_SEAL_WRITE is set,
    // which seems contrary to the manpage for fcntl(2).
    s_log = static_cast<log*>(map_fd(sizeof(log), PROT_READ, MAP_PRIVATE, fd_log, 0));
    // Actually commit the transaction.
    bool success = txn_commit();
    session_event_t decision = success ? session_event_t::DECIDE_TXN_COMMIT : session_event_t::DECIDE_TXN_ABORT;
    // Server-initiated state transition! (Any issues with reentrant handlers?)
    apply_transition(decision, nullptr, nullptr, 0);
}

void server::handle_decide_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(
        event == session_event_t::DECIDE_TXN_COMMIT || event == session_event_t::DECIDE_TXN_ABORT,
        "Unexpected event received!");
    retail_assert(
        old_state == session_state_t::TXN_COMMITTING && new_state == session_state_t::CONNECTED,
        "Current event is inconsistent with state transition!");
    FlatBufferBuilder builder;
    build_server_reply(builder, event, old_state, new_state, s_txn_id);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());
    s_txn_id = 0;
}

void server::handle_client_shutdown(
    int*, size_t, session_event_t event, const void*, session_state_t, session_state_t new_state)
{
    retail_assert(event == session_event_t::CLIENT_SHUTDOWN, "Unexpected event received!");
    retail_assert(
        new_state == session_state_t::DISCONNECTED,
        "Current event is inconsistent with state transition!");
    // If this event is received, the client must have closed the write end of the socket
    // (equivalent of sending a FIN), so we need to do the same. Closing the socket
    // will send a FIN to the client, so they will read EOF and can close the socket
    // as well. We can just set the shutdown flag here, which will break out of the poll
    // loop and immediately close the socket. (If we received EOF from the client after
    // we closed our write end, then we would be calling shutdown(SHUT_WR) twice, which
    // is another reason to just close the socket.)
    s_session_shutdown = true;
}

void server::handle_server_shutdown(
    int*, size_t, session_event_t event, const void*, session_state_t, session_state_t new_state)
{
    retail_assert(event == session_event_t::SERVER_SHUTDOWN, "Unexpected event received!");
    retail_assert(
        new_state == session_state_t::DISCONNECTED,
        "Current event is inconsistent with state transition!");
    // This transition should only be triggered on notification of the server shutdown event.
    // Since we are about to shut down, we can't wait for acknowledgment from the client and
    // should just close the session socket. As noted above, setting the shutdown flag will
    // immediately break out of the poll loop and close the session socket.
    s_session_shutdown = true;
}

void server::handle_request_stream(
    int*, size_t, session_event_t event, const void* event_data, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::REQUEST_STREAM, "Unexpected event received!");
    // This event never changes session state.
    retail_assert(
        old_state == new_state,
        "Current event is inconsistent with state transition!");
    // Create a connected pair of datagram sockets, one of which we will keep
    // and the other we will send to the client.
    // We use SOCK_SEQPACKET because it supports both datagram and connection
    // semantics: datagrams allow buffering without framing, and a connection
    // ensures that client returns EOF after server has called shutdown(SHUT_WR).
    int socket_pair[2];
    constexpr int c_server_index = 0;
    constexpr int c_client_index = 1;
    if (-1 == ::socketpair(PF_UNIX, SOCK_SEQPACKET, 0, socket_pair))
    {
        throw_system_error("socketpair failed");
    }
    int server_socket = socket_pair[c_server_index];
    int client_socket = socket_pair[c_client_index];
    auto socket_cleanup = make_scope_guard([&]() {
        close_fd(server_socket);
        close_fd(client_socket);
    });
    // Set server socket to be nonblocking, since we use it within an epoll loop.
    int flags = ::fcntl(server_socket, F_GETFL);
    if (flags == -1)
    {
        throw_system_error("fcntl(F_GETFL) failed");
    }
    if (-1 == ::fcntl(server_socket, F_SETFL, flags | O_NONBLOCK))
    {
        throw_system_error("fcntl(F_SETFL) failed");
    }
    // The only currently supported stream type is table scans.
    // When we add more stream types, we should add a switch statement on data_type.
    // It would be nice to delegate to a helper returning a different generator for each
    // data_type, and then invoke start_stream_producer() with that generator, but in
    // general each data_type corresponds to a generator with a different T_element_type,
    // so we need to invoke start_stream_producer() separately for each data_type
    // (because start_stream_producer() is templated on the generator's T_element_type).
    // We should logically receive an object corresponding to the request_data_t union,
    // but the FlatBuffers API doesn't have any object corresponding to a union.
    auto request = static_cast<const client_request_t*>(event_data);
    retail_assert(
        request->data_type() == request_data_t::table_scan,
        "Unexpected request data type");
    auto type = static_cast<gaia_type_t>(request->data_as_table_scan()->type_id());
    auto id_generator = get_id_generator_for_type(type);
    start_stream_producer(server_socket, id_generator);
    // Any exceptions after this point will close the server socket, ensuring the producer thread terminates.
    // However, its destructor will not run until the session thread exits and joins the producer thread.
    FlatBufferBuilder builder;
    build_server_reply(builder, event, old_state, new_state);
    send_msg_with_fds(s_session_socket, &client_socket, 1, builder.GetBufferPointer(), builder.GetSize());
    // Close the client socket in our process since we duplicated it.
    close_fd(client_socket);
    socket_cleanup.dismiss();
}

void server::apply_transition(session_event_t event, const void* event_data, int* fds, size_t fd_count)
{
    if (event == session_event_t::NOP)
    {
        return;
    }
    // "Wildcard" transitions (current state = session_state_t::ANY) must be listed after
    // non-wildcard transitions with the same event, or the latter will never be applied.
    for (auto t : s_valid_transitions)
    {
        if (t.event == event && (t.state == s_session_state || t.state == session_state_t::ANY))
        {
            session_state_t new_state = t.transition.new_state;
            // If the transition's new state is ANY, then keep the state the same.
            if (new_state == session_state_t::ANY)
            {
                new_state = s_session_state;
            }
            session_state_t old_state = s_session_state;
            s_session_state = new_state;
            if (t.transition.handler)
            {
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
        + "' with event '"
        + std::string(EnumNamesession_event_t(event))
        + "'");
}

void server::build_server_reply(
    FlatBufferBuilder& builder, session_event_t event, session_state_t old_state, session_state_t new_state, gaia_txn_id_t txn_id)
{
    const auto server_reply = [&] {
        if (txn_id)
        {
            const auto transaction_info = Createtransaction_info_t(builder, txn_id);
            return Createserver_reply_t(
                builder, event, old_state, new_state, reply_data_t::transaction_info, transaction_info.Union());
        }
        else
        {
            return Createserver_reply_t(builder, event, old_state, new_state);
        }
    }();
    const auto message = Createmessage_t(builder, any_message_t::reply, server_reply.Union());
    builder.Finish(message);
}

void server::clear_shared_memory()
{
    if (s_shared_locators)
    {
        unmap_fd(s_shared_locators, sizeof(locators));
    }
    if (s_fd_locators != -1)
    {
        close_fd(s_fd_locators);
    }
    if (s_data)
    {
        unmap_fd(s_data, sizeof(data));
    }
    if (s_fd_data != -1)
    {
        close_fd(s_fd_data);
    }
}

// To avoid synchronization, we assume that this method is only called when
// no sessions exist and the server is not accepting any new connections.
void server::init_shared_memory()
{
    // The listening socket must not be open.
    retail_assert(s_listening_socket == -1, "Listening socket should not be open!");
    // We may be reinitializing the server upon receiving a SIGHUP.
    clear_shared_memory();
    // Clear all shared memory if an exception is thrown.
    auto cleanup_memory = make_scope_guard([]() { clear_shared_memory(); });
    retail_assert(s_fd_data == -1 && s_fd_locators == -1, "Data and locator fds should not be valid!");
    retail_assert(!s_data && !s_shared_locators, "Data and locators memory should be reset!");
    s_fd_locators = ::memfd_create(c_sch_mem_locators, MFD_ALLOW_SEALING);
    if (s_fd_locators == -1)
    {
        throw_system_error("memfd_create failed");
    }
    s_fd_data = ::memfd_create(c_sch_mem_data, MFD_ALLOW_SEALING);
    if (s_fd_data == -1)
    {
        throw_system_error("memfd_create failed");
    }
    if (-1 == ::ftruncate(s_fd_locators, sizeof(locators)) || -1 == ::ftruncate(s_fd_data, sizeof(data)))
    {
        throw_system_error("ftruncate failed");
    }
    s_shared_locators = static_cast<locators*>(map_fd(sizeof(locators), PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_locators, 0));
    s_data = static_cast<data*>(map_fd(sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_data, 0));
    // Populate shared memory from the persistent log and snapshot.
    recover_db();
    cleanup_memory.dismiss();
}

void server::recover_db()
{
    // If persistence is disabled, then this is a no-op.
    if (!s_disable_persistence)
    {
        // Open RocksDB just once.
        if (!rdb)
        {
            rdb = make_unique<gaia::db::persistent_store_manager>();
            rdb->open();
            rdb->recover();
        }
    }
}

sigset_t server::mask_signals()
{
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

void server::signal_handler(sigset_t sigset, int& signum)
{
    // Wait until a signal is delivered.
    // REVIEW: do we have any use for sigwaitinfo()?
    ::sigwait(&sigset, &signum);
    cerr << "Caught signal " << ::strsignal(signum) << endl;
    // Signal the eventfd by writing a nonzero value.
    // This value is large enough that no thread will
    // decrement it to zero, so every waiting thread
    // should see a nonzero value.
    ssize_t bytes_written = ::write(s_server_shutdown_eventfd, &MAX_SEMAPHORE_COUNT, sizeof(MAX_SEMAPHORE_COUNT));
    if (bytes_written == -1)
    {
        throw_system_error("write to eventfd failed");
    }
    retail_assert(bytes_written == sizeof(MAX_SEMAPHORE_COUNT), "Failed to fully write data!");
}

void server::init_listening_socket()
{
    // Launch a connection-based listening Unix socket on a well-known address.
    // We use SOCK_SEQPACKET to get connection-oriented *and* datagram semantics.
    // This socket needs to be nonblocking so we can use epoll to wait on the
    // shutdown eventfd as well (the connected sockets it spawns will inherit
    // nonblocking mode). Actually, nonblocking mode may not have any effect in
    // level-triggered epoll mode, but it's good to ensure we can never block,
    // in case of bugs or surprising semantics in epoll.
    int listening_socket = ::socket(PF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
    if (listening_socket == -1)
    {
        throw_system_error("socket creation failed");
    }
    auto socket_cleanup = make_scope_guard([&]() { close_fd(listening_socket); });

    // Initialize the socket address structure.
    sockaddr_un server_addr = {0};
    server_addr.sun_family = AF_UNIX;
    // The socket name (minus its null terminator) needs to fit into the space
    // in the server address structure after the prefix null byte.
    static_assert(sizeof(SE_SERVER_SOCKET_NAME) <= sizeof(server_addr.sun_path) - 1);
    // We prepend a null byte to the socket name so the address is in the
    // (Linux-exclusive) "abstract namespace", i.e., not bound to the
    // filesystem.
    ::strncpy(&server_addr.sun_path[1], SE_SERVER_SOCKET_NAME, sizeof(server_addr.sun_path) - 1);

    // Bind the socket to the address and start listening for connections.
    // The socket name is not null-terminated in the address structure, but
    // we need to add an extra byte for the null byte prefix.
    socklen_t server_addr_size = sizeof(server_addr.sun_family) + 1 + ::strlen(&server_addr.sun_path[1]);
    if (-1 == ::bind(listening_socket, reinterpret_cast<struct sockaddr*>(&server_addr), server_addr_size))
    {
        throw_system_error("bind failed");
    }
    if (-1 == ::listen(listening_socket, 0))
    {
        throw_system_error("listen failed");
    }

    socket_cleanup.dismiss();
    s_listening_socket = listening_socket;
}

bool server::authenticate_client_socket(int socket)
{
    struct ucred cred;
    socklen_t cred_len = sizeof(cred);
    if (-1 == ::getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len))
    {
        throw_system_error("getsockopt(SO_PEERCRED) failed");
    }
    // Disable client authentication until we can figure out
    // how to fix the Postgres tests.
    // Client must have same effective user ID as server.
    // return (cred.uid == ::geteuid());
    return true;
}

void server::client_dispatch_handler()
{
    // Initialize session thread list first, so we can clean it up last.
    std::vector<std::thread> session_threads;
    auto session_cleanup = make_scope_guard([&session_threads]() {
        for (std::thread& t : session_threads)
        {
            t.join();
        }
    });

    // Start listening for incoming client connections.
    init_listening_socket();
    // We close the listening socket before waiting for session threads to exit,
    // so no new sessions can be established while we wait for all session
    // threads to exit (we assume they received the same server shutdown
    // notification that we did).
    auto listener_cleanup = make_scope_guard([&]() {
        close_fd(s_listening_socket);
    });

    // Set up the epoll loop.
    int epoll_fd = ::epoll_create1(0);
    if (epoll_fd == -1)
    {
        throw_system_error("epoll_create1 failed");
    }
    // We close the epoll descriptor before closing the listening socket, so any
    // connections that arrive before the listening socket is closed will
    // receive ECONNRESET rather than ECONNREFUSED. This is perhaps unfortunate
    // but shouldn't really matter in practice.
    auto epoll_cleanup = make_scope_guard([&]() { close_fd(epoll_fd); });
    int registered_fds[] = {s_listening_socket, s_server_shutdown_eventfd};
    for (int registered_fd : registered_fds)
    {
        epoll_event ev = {0};
        ev.events = EPOLLIN;
        ev.data.fd = registered_fd;
        if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, registered_fd, &ev))
        {
            throw_system_error("epoll_ctl failed");
        }
    }
    epoll_event events[std::size(registered_fds)];

    // Enter the epoll loop.
    while (true)
    {
        // Block forever (we will be notified of shutdown).
        int ready_fd_count = ::epoll_wait(epoll_fd, events, std::size(events), -1);
        if (ready_fd_count == -1)
        {
            // Attaching the debugger will send a SIGSTOP which we can't block.
            // Any signal which we block will set the shutdown eventfd and will
            // alert the epoll fd, so we don't have to worry about getting EINTR
            // from a signal intended to terminate the process.
            if (errno == EINTR)
            {
                continue;
            }
            throw_system_error("epoll_wait failed");
        }
        for (int i = 0; i < ready_fd_count; i++)
        {
            epoll_event ev = events[i];
            // We never register for anything but EPOLLIN,
            // but EPOLLERR will always be delivered.
            if (ev.events & EPOLLERR)
            {
                if (ev.data.fd == s_listening_socket)
                {
                    int error = 0;
                    socklen_t err_len = sizeof(error);
                    // Ignore errors getting error message and default to generic error message.
                    ::getsockopt(s_listening_socket, SOL_SOCKET, SO_ERROR, static_cast<void*>(&error), &err_len);
                    throw_system_error("client socket error", error);
                }
                else if (ev.data.fd == s_server_shutdown_eventfd)
                {
                    throw_system_error("shutdown eventfd error");
                }
            }
            // At this point, we should only get EPOLLIN.
            retail_assert(ev.events == EPOLLIN, "Unexpected event type!");
            if (ev.data.fd == s_listening_socket)
            {
                int session_socket = ::accept(s_listening_socket, nullptr, nullptr);
                if (session_socket == -1)
                {
                    throw_system_error("accept failed");
                }
                if (authenticate_client_socket(session_socket))
                {
                    session_threads.emplace_back(session_handler, session_socket);
                }
                else
                {
                    close_fd(session_socket);
                }
            }
            else if (ev.data.fd == s_server_shutdown_eventfd)
            {
                uint64_t val;
                ssize_t bytes_read = ::read(s_server_shutdown_eventfd, &val, sizeof(val));
                if (bytes_read == -1)
                {
                    throw_system_error("read failed");
                }
                // We should always read the value 1 from a semaphore eventfd.
                retail_assert(bytes_read == sizeof(val) && val == 1, "Unexpected value read from semaphore event fd");
                return;
            }
            else
            {
                // We don't monitor any other fds.
                retail_assert(false, "Unexpected event fd type detected!");
            }
        }
    }
}

void server::session_handler(int session_socket)
{
    // Set up session socket.
    s_session_shutdown = false;
    s_session_socket = session_socket;
    auto socket_cleanup = make_scope_guard([&]() {
        // We can rely on close_fd() to perform the equivalent of
        // shutdown(SHUT_RDWR), since we hold the only fd pointing to this
        // socket. That should allow the client to read EOF if they're in a
        // blocking read and exit gracefully. (If they try to write to the
        // socket after we've closed our end, they'll receive EPIPE.) We don't
        // want to try to read any pending data from the client, since we're
        // trying to shut down as quickly as possible.
        close_fd(s_session_socket);
    });

    // Set up epoll loop.
    int epoll_fd = ::epoll_create1(0);
    if (epoll_fd == -1)
    {
        throw_system_error("epoll_create1 failed");
    }
    auto epoll_cleanup = make_scope_guard([&]() { close_fd(epoll_fd); });
    int fds[] = {s_session_socket, s_server_shutdown_eventfd};
    for (int fd : fds)
    {
        // We should only get EPOLLRDHUP from the client socket, but oh well.
        epoll_event ev = {0};
        ev.events = EPOLLIN | EPOLLRDHUP;
        ev.data.fd = fd;
        if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev))
        {
            throw_system_error("epoll_ctl failed");
        }
    }
    epoll_event events[std::size(fds)];
    // Event to signal session-owned threads to terminate.
    int session_shutdown_eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
    if (session_shutdown_eventfd == -1)
    {
        throw_system_error("eventfd failed");
    }
    s_session_shutdown_eventfd = session_shutdown_eventfd;
    auto owned_threads_cleanup = make_scope_guard([]() {
        // Signal the eventfd by writing a nonzero value.
        // This value is large enough that no thread will
        // decrement it to zero, so every waiting thread
        // should see a nonzero value.
        ssize_t bytes_written = ::write(s_session_shutdown_eventfd, &MAX_SEMAPHORE_COUNT, sizeof(MAX_SEMAPHORE_COUNT));
        if (bytes_written == -1)
        {
            throw_system_error("write to eventfd failed");
        }
        retail_assert(bytes_written == sizeof(MAX_SEMAPHORE_COUNT), "Failed to fully write data!");
        // Wait for all session-owned threads to terminate.
        for (std::thread& t : s_session_owned_threads)
        {
            t.join();
        }
    });

    // Enter epoll loop.
    while (!s_session_shutdown)
    {
        // Block forever (we will be notified of shutdown).
        int ready_fd_count = ::epoll_wait(epoll_fd, events, std::size(events), -1);
        if (ready_fd_count == -1)
        {
            // Attaching the debugger will send a SIGSTOP which we can't block.
            // Any signal which we block will set the shutdown eventfd and will
            // alert the epoll fd, so we don't have to worry about getting EINTR
            // from a signal intended to terminate the process.
            if (errno == EINTR)
            {
                continue;
            }
            throw_system_error("epoll_wait failed");
        }
        session_event_t event = session_event_t::NOP;
        const void* event_data = nullptr;
        int* fds = nullptr;
        size_t fd_count = 0;
        for (int i = 0; i < ready_fd_count; i++)
        {
            epoll_event ev = events[i];
            if (ev.data.fd == s_session_socket)
            {
                // NB: Since many event flags are set in combination with others, the
                // order we test them in matters! E.g., EPOLLIN seems to always be set
                // whenever EPOLLRDHUP is set, so we need to test EPOLLRDHUP before
                // testing EPOLLIN.
                if (ev.events & EPOLLERR)
                {
                    // This flag is unmaskable, so we don't need to register for it.
                    int error = 0;
                    socklen_t err_len = sizeof(error);
                    // Ignore errors getting error message and default to generic error message.
                    ::getsockopt(s_session_socket, SOL_SOCKET, SO_ERROR, static_cast<void*>(&error), &err_len);
                    cerr << "client socket error: " << ::strerror(error) << endl;
                    event = session_event_t::CLIENT_SHUTDOWN;
                }
                else if (ev.events & EPOLLHUP)
                {
                    // This flag is unmaskable, so we don't need to register for it.
                    // Both ends of the socket have issued a shutdown(SHUT_WR) or equivalent.
                    retail_assert(!(ev.events & EPOLLERR), "EPOLLERR flag should not be set!");
                    event = session_event_t::CLIENT_SHUTDOWN;
                }
                else if (ev.events & EPOLLRDHUP)
                {
                    // The client has called shutdown(SHUT_WR) to signal their intention to
                    // disconnect. We do the same by closing the session socket.
                    // REVIEW: Can we get both EPOLLHUP and EPOLLRDHUP when the client half-closes
                    // the socket after we half-close it?
                    retail_assert(!(ev.events & EPOLLHUP), "EPOLLHUP flag should not be set!");
                    event = session_event_t::CLIENT_SHUTDOWN;
                }
                else if (ev.events & EPOLLIN)
                {
                    retail_assert(
                        !(ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)),
                        "EPOLLERR, EPOLLHUP, EPOLLRDHUP flags should not be set!");
                    // Buffer used to send and receive all message data.
                    uint8_t msg_buf[c_max_msg_size] = {0};
                    // Buffer used to receive file descriptors.
                    int fd_buf[c_max_fd_count] = {-1};
                    size_t fd_buf_size = std::size(fd_buf);
                    // Read client message with possible file descriptors.
                    size_t bytes_read = recv_msg_with_fds(s_session_socket, fd_buf, &fd_buf_size, msg_buf, sizeof(msg_buf));
                    // We shouldn't get EOF unless EPOLLRDHUP is set.
                    // REVIEW: it might be possible for the client to call shutdown(SHUT_WR)
                    // after we have already woken up on EPOLLIN, in which case we would
                    // legitimately read 0 bytes and this assert would be invalid.
                    retail_assert(bytes_read > 0, "Failed to read message!");
                    const message_t* msg = Getmessage_t(msg_buf);
                    const client_request_t* request = msg->msg_as_request();
                    event = request->event();
                    // We need to pass auxiliary data identifying the requested stream type and properties.
                    if (event == session_event_t::REQUEST_STREAM)
                    {
                        // We should logically pass an object corresponding to the request_data_t union,
                        // but the FlatBuffers API doesn't have any object corresponding to a union.
                        event_data = static_cast<const void*>(request);
                    }
                    if (fd_buf_size > 0)
                    {
                        fds = fd_buf;
                        fd_count = fd_buf_size;
                    }
                }
                else
                {
                    // We don't register for any other events.
                    retail_assert(false, "Unexpected event type!");
                }
            }
            else if (ev.data.fd == s_server_shutdown_eventfd)
            {
                retail_assert(ev.events == EPOLLIN, "Expected EPOLLIN event type!");
                // We should always read the value 1 from a semaphore eventfd.
                uint64_t val;
                ssize_t bytes_read = ::read(s_server_shutdown_eventfd, &val, sizeof(val));
                if (bytes_read == -1)
                {
                    throw_system_error("read(eventfd) failed");
                }
                retail_assert(bytes_read == sizeof(val), "Failed to fully read expected data!");
                retail_assert(val == 1, "Unexpected value received!");
                event = session_event_t::SERVER_SHUTDOWN;
            }
            else
            {
                // We don't monitor any other fds.
                retail_assert(false, "Unexpected event fd!");
            }
            retail_assert(event != session_event_t::NOP, "Unexpected event type!");
            apply_transition(event, event_data, fds, fd_count);
        }
    }
}

template <typename T_element_type>
void server::stream_producer_handler(
    int stream_socket, int cancel_eventfd, std::function<std::optional<T_element_type>()> generator_fn)
{
    // We only support fixed-width integer types for now to avoid framing.
    static_assert(std::is_integral<T_element_type>::value, "Generator function must return an integer.");
    // Verify the socket is the correct type for the semantics we assume.
    check_socket_type(stream_socket, SOCK_SEQPACKET);
    auto gen_iter = generator_iterator_t<T_element_type>(generator_fn);
    auto socket_cleanup = make_scope_guard([&]() {
        // We can rely on close_fd() to perform the equivalent of shutdown(SHUT_RDWR),
        // since we hold the only fd pointing to this socket.
        close_fd(stream_socket);
    });
    // Check if our stream socket is non-blocking (so we don't accidentally block in write()).
    int flags = ::fcntl(stream_socket, F_GETFL, 0);
    if (flags == -1)
    {
        throw_system_error("fcntl(F_GETFL) failed");
    }
    retail_assert(flags & O_NONBLOCK, "Stream socket is in blocking mode!");
    int epoll_fd = ::epoll_create1(0);
    if (epoll_fd == -1)
    {
        throw_system_error("epoll_create1 failed");
    }
    auto epoll_cleanup = make_scope_guard([&]() { close_fd(epoll_fd); });
    // We poll for write availability of the stream socket in level-triggered mode,
    // and only write at most one buffer of data before polling again, to avoid read
    // starvation of the cancellation eventfd.
    epoll_event sock_ev = {0};
    sock_ev.events = EPOLLOUT;
    sock_ev.data.fd = stream_socket;
    if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stream_socket, &sock_ev))
    {
        throw_system_error("epoll_ctl failed");
    }
    epoll_event cancel_ev = {0};
    cancel_ev.events = EPOLLIN;
    cancel_ev.data.fd = cancel_eventfd;
    if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cancel_eventfd, &cancel_ev))
    {
        throw_system_error("epoll_ctl failed");
    }
    epoll_event events[2];
    bool producer_shutdown = false;
    // The userspace buffer that we use to construct a batch datagram message.
    std::vector<T_element_type> batch_buffer;
    // We need to call reserve() rather than the "sized" constructor to avoid changing size().
    batch_buffer.reserve(STREAM_BATCH_SIZE);
    while (!producer_shutdown)
    {
        // Block forever (we will be notified of shutdown).
        int ready_fd_count = ::epoll_wait(epoll_fd, events, std::size(events), -1);
        if (ready_fd_count == -1)
        {
            // Attaching the debugger will send a SIGSTOP which we can't block.
            // Any signal which we block will set the shutdown eventfd and will
            // alert the epoll fd, so we don't have to worry about getting EINTR
            // from a signal intended to terminate the process.
            if (errno == EINTR)
            {
                continue;
            }
            throw_system_error("epoll_wait failed");
        }
        for (int i = 0; i < ready_fd_count; i++)
        {
            epoll_event ev = events[i];
            if (ev.data.fd == stream_socket)
            {
                // NB: Since many event flags are set in combination with others, the
                // order we test them in matters! E.g., EPOLLIN seems to always be set
                // whenever EPOLLRDHUP is set, so we need to test EPOLLRDHUP before
                // testing EPOLLIN.
                if (ev.events & EPOLLERR)
                {
                    // This flag is unmaskable, so we don't need to register for it.
                    int error = 0;
                    socklen_t err_len = sizeof(error);
                    // Ignore errors getting error message and default to generic error message.
                    ::getsockopt(stream_socket, SOL_SOCKET, SO_ERROR, static_cast<void*>(&error), &err_len);
                    cerr << "stream socket error: " << ::strerror(error) << endl;
                    producer_shutdown = true;
                }
                else if (ev.events & EPOLLHUP)
                {
                    // This flag is unmaskable, so we don't need to register for it.
                    // We shold get this when the client has closed its end of the socket.
                    retail_assert(!(ev.events & EPOLLERR), "EPOLLERR flag should not be set!");
                    producer_shutdown = true;
                }
                else if (ev.events & EPOLLOUT)
                {
                    retail_assert(
                        !(ev.events & (EPOLLERR | EPOLLHUP)),
                        "EPOLLERR and EPOLLHUP flags should not be set!");
                    // Write to the send buffer until we exhaust either the iterator or the buffer free space.
                    while (gen_iter && (batch_buffer.size() < STREAM_BATCH_SIZE))
                    {
                        T_element_type next_val = *gen_iter;
                        batch_buffer.push_back(next_val);
                        ++gen_iter;
                    }
                    // We need to send any pending data in the buffer, followed by EOF
                    // if we reached end of iteration. We let the client decide when to
                    // close the socket, since their next read may be arbitrarily delayed
                    // (and they may still have pending data).
                    // First send any remaining data in the buffer.
                    if (batch_buffer.size() > 0)
                    {
                        // To simplify client state management by allowing the client to
                        // dequeue entries in FIFO order using std::vector.pop_back(),
                        // we reverse the order of entries in the buffer.
                        std::reverse(std::begin(batch_buffer), std::end(batch_buffer));
                        // We don't want to handle signals, so set
                        // MSG_NOSIGNAL to convert SIGPIPE to EPIPE.
                        ssize_t bytes_written = ::send(
                            stream_socket, batch_buffer.data(), batch_buffer.size() * sizeof(T_element_type),
                            MSG_NOSIGNAL);
                        if (bytes_written == -1)
                        {
                            // It should never happen that the socket is no longer writable
                            // after we receive EPOLLOUT, since we are the only writer and
                            // the receive buffer is always large enough for a batch.
                            retail_assert(errno != EAGAIN && errno != EWOULDBLOCK, "Unexpected errno value!");
                            // Log the error and break out of the poll loop.
                            cerr << "stream socket error: " << ::strerror(errno) << endl;
                            producer_shutdown = true;
                        }
                        else
                        {
                            // We successfully wrote to the socket, so clear the buffer.
                            // (Partial writes are impossible with datagram sockets.)
                            // The standard is somewhat unclear, but apparently clear() will
                            // not change the capacity in any recent implementation of the
                            // standard library (https://cplusplus.github.io/LWG/issue1102).
                            batch_buffer.clear();
                        }
                    }
                    // If we reached end of iteration, send EOF to client.
                    // (We still need to wait for the client to close their socket,
                    // since they may still have unread data, so we don't set the
                    // producer_shutdown flag.)
                    if (!gen_iter)
                    {
                        ::shutdown(stream_socket, SHUT_WR);
                    }
                }
                else
                {
                    // We don't register for any other events.
                    retail_assert(false, "Unexpected event type!");
                }
            }
            else if (ev.data.fd == cancel_eventfd)
            {
                retail_assert(ev.events == EPOLLIN, "Unexpected event type!");
                // We should always read the value 1 from a semaphore eventfd.
                uint64_t val;
                ssize_t bytes_read = ::read(cancel_eventfd, &val, sizeof(val));
                if (bytes_read == -1)
                {
                    throw_system_error("read(eventfd) failed");
                }
                retail_assert(bytes_read == sizeof(val), "Failed to fully read data!");
                retail_assert(val == 1, "Unexpected data received!");
                producer_shutdown = true;
            }
            else
            {
                // We don't monitor any other fds.
                retail_assert(false, "Unexpected event fd!");
            }
        }
    }
}

template <typename T_element_type>
void server::start_stream_producer(int stream_socket, std::function<std::optional<T_element_type>()> generator_fn)
{
    // Give the session ownership of the new stream thread.
    s_session_owned_threads.emplace_back(
        stream_producer_handler<T_element_type>, stream_socket, s_session_shutdown_eventfd, generator_fn);
}

std::function<std::optional<gaia_id_t>()> server::get_id_generator_for_type(gaia_type_t type)
{
    gaia_locator_t locator = 0;
    return [type, locator]() mutable -> std::optional<gaia_id_t> {
        // We need to ensure that we're not reading the locator segment
        // while a committing transaction is writing to it.
        std::shared_lock lock(s_locators_lock);
        while (++locator && locator < s_data->locator_count + 1)
        {
            gaia_offset_t offset = (*s_shared_locators)[locator];
            if (offset)
            {
                auto obj = reinterpret_cast<gaia_se_object_t*>(s_data->objects + (*s_shared_locators)[locator]);
                if (obj->type == type)
                {
                    return obj->id;
                }
            }
        }
        // Signal end of iteration.
        return std::nullopt;
    };
}

// Before this method is called, we have already received the log fd from the client
// and mmapped it.
// This method returns true for a commit decision and false for an abort decision.
bool server::txn_commit()
{
    // At the process level, acquiring an advisory file lock in exclusive mode
    // guarantees there are no clients mapping the locator segment. It does not
    // guarantee there are no other threads in this process that have acquired
    // an exclusive lock, though (hence the additional mutex).
    if (-1 == ::flock(s_fd_locators, LOCK_EX))
    {
        throw_system_error("flock failed");
    }
    // Within our own process, we must have exclusive access to the locator segment.
    std::unique_lock lock(s_locators_lock);
    auto cleanup = make_scope_guard([&]() {
        if (-1 == ::flock(s_fd_locators, LOCK_UN))
        {
            // Per C++11 semantics, throwing an exception from a destructor
            // will just call std::terminate(), no undefined behavior.
            throw_system_error("flock failed");
        }
        unmap_fd(s_log, sizeof(s_log));
    });

    std::set<gaia_locator_t> locators;
    // This is only used for persistence.
    std::string txn_name;

    if (!s_disable_persistence)
    {
        txn_name = rdb->begin_txn(s_txn_id);
        // Prepare log for transaction.
        rdb->prepare_wal_for_write(txn_name);
    }

    for (size_t i = 0; i < s_log->count; i++)
    {
        auto lr = s_log->log_records + i;

        if (locators.insert(lr->locator).second)
        {
            if ((*s_shared_locators)[lr->locator] != lr->old_offset)
            {
                if (!s_disable_persistence)
                {
                    // Append rollback decision to log.
                    // This isn't really required because recovery will skip deserializing transactions
                    // that don't have a commit marker; we do it for completeness anyway.
                    rdb->append_wal_rollback_marker(txn_name);
                }
                return false;
            }
        }
    }

    for (size_t i = 0; i < s_log->count; i++)
    {
        auto lr = s_log->log_records + i;
        (*s_shared_locators)[lr->locator] = lr->new_offset;
    }

    if (!s_disable_persistence)
    {
        // Append commit decision to the log.
        rdb->append_wal_commit_marker(txn_name);
    }

    return true;
}

// this must be run on main thread
// see https://thomastrapp.com/blog/signal-handler-for-multithreaded-c++/
void server::run(bool disable_persistence)
{
    // There can only be one thread running at this point, so this doesn't need synchronization.
    s_disable_persistence = disable_persistence;
    // Block handled signals in this thread and subsequently spawned threads.
    sigset_t handled_signals = mask_signals();
    while (true)
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
        int server_shutdown_eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
        if (server_shutdown_eventfd == -1)
        {
            throw_system_error("eventfd failed");
        }
        s_server_shutdown_eventfd = server_shutdown_eventfd;
        // Launch signal handler thread.
        int caught_signal = 0;
        std::thread signal_handler_thread(signal_handler, handled_signals, std::ref(caught_signal));
        init_shared_memory();
        std::thread client_dispatch_thread(client_dispatch_handler);
        // The client dispatch thread will only return after all sessions have been disconnected
        // and the listening socket has been closed.
        client_dispatch_thread.join();
        // The signal handler thread will only return after a blocked signal is pending.
        signal_handler_thread.join();
        // We can't close this fd until all readers and writers have exited.
        // The only readers are the client dispatch thread and the session
        // threads, and the only writer is the signal handler thread.
        close_fd(s_server_shutdown_eventfd);
        // We shouldn't get here unless the signal handler thread has caught a signal.
        retail_assert(caught_signal != 0, "A signal should have been caught!");
        // We special-case SIGHUP to force reinitialization of the server.
        if (caught_signal != SIGHUP)
        {
            // To exit with the correct status (reflecting a caught signal),
            // we need to unblock blocked signals and re-raise the signal.
            // We may have already received other pending signals by the time
            // we unblock signals, in which case they will be delivered and
            // terminate the process before we can re-raise the caught signal.
            // That is benign, since we've already performed cleanup actions
            // and the exit status will still be valid.
            ::pthread_sigmask(SIG_UNBLOCK, &handled_signals, nullptr);
            ::raise(caught_signal);
        }
    }
}
