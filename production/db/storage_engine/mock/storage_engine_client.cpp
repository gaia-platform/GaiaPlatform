/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine_client.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::messages;
using namespace flatbuffers;

thread_local se_base::offsets* client::s_offsets = nullptr;
thread_local int client::s_fd_log = -1;

std::atomic<gaia_tx_hook> client::s_tx_begin_hook {nullptr};
std::atomic<gaia_tx_hook> client::s_tx_commit_hook{nullptr};
std::atomic<gaia_tx_hook> client::s_tx_rollback_hook{nullptr};

static void build_client_request(FlatBufferBuilder& builder, session_event_t event) {
    auto client_request = Createclient_request_t(builder, event);
    auto message = Createmessage_t(builder, any_message_t::request, client_request.Union());
    builder.Finish(message);
}

void client::destroy_log_mapping() {
    // We might have already destroyed the log mapping before committing the txn.
    if (s_log) {
        unmap_fd(s_log, sizeof(log));
        s_log = nullptr;
    }
}

void client::tx_cleanup() {
    // Destroy the log memory mapping.
    destroy_log_mapping();
    // Destroy the log fd.
    close(s_fd_log);
    s_fd_log = -1;
    // Destroy the offset mapping.
    unmap_fd(s_offsets, sizeof(offsets));
    s_offsets = nullptr;
}

int client::get_session_socket() {
    // Unlike the session socket on the server, this socket must be blocking,
    // since we don't read within a multiplexing poll loop.
    int session_socket = socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (session_socket == -1) {
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
    if (-1 == connect(session_socket, (struct sockaddr *)&server_addr,
                server_addr_size)) {
        throw_system_error("connect failed");
    }
    return session_socket;
}

// This method establishes a session with the server on the calling thread,
// which serves as a transaction context in the client and a container for
// client state on the server. The server always passes fds for the data and
// locator shared memory segments, but we only use them if this is the client
// process's first call to create_session(), since they are stored globally
// rather than per-session. (The connected socket is stored per-session.)
void client::begin_session()
{
    // Fail if a session already exists on this thread.
    verify_no_session();

    // Connect to the server's well-known socket name, and ask it
    // for the data and locator shared memory segment fds.
    s_session_socket = get_session_socket();

    // Send the server the connection request.
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::CONNECT);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Extract the data and locator shared memory segment fds from the server's response.
    uint8_t msg_buf[MAX_MSG_SIZE] = {0};
    const size_t FD_COUNT = 2;
    int fds[FD_COUNT] = {-1};
    size_t fd_count = FD_COUNT;
    const size_t DATA_FD_INDEX = 0;
    const size_t OFFSETS_FD_INDEX = 1;
    size_t bytes_read = recv_msg_with_fds(s_session_socket, fds, &fd_count, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0);
    retail_assert(fd_count == FD_COUNT);
    const message_t *msg = Getmessage_t(msg_buf);
    const server_reply_t *reply = msg->msg_as_reply();
    const session_event_t event = reply->event();
    retail_assert(event == session_event_t::CONNECT);
    // Since the data and locator fds are global, we need to atomically update them
    // (and only if they're not already initialized).
    int fd_data = fds[DATA_FD_INDEX];
    retail_assert(fd_data != -1);

    // Set up the shared data segment mapping.
    if (!s_data) {
        data* data_mapping = static_cast<data*>(map_fd(
            sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0));
        if (!__sync_bool_compare_and_swap(&s_data, 0, data_mapping)) {
            // we lost the race, throw away the mapping
            unmap_fd(data_mapping, sizeof(data));
        }
    }
    // We've already mapped the data fd, so we can close it now.
    close(fd_data);

    // Set up the private locator segment fd.
    int fd_offsets = fds[OFFSETS_FD_INDEX];
    retail_assert(fd_offsets != -1);
    if (s_fd_offsets == -1) {
        if (!__sync_bool_compare_and_swap(&s_fd_offsets, -1, fd_offsets)) {
            // we lost the race, close the fd
            close(fd_offsets);
        }
    } else {
        // locator fd is already initialized, close the fd
        close(fd_offsets);
    }
}

void client::end_session()
{
    // Send the server EOF.
    shutdown(s_session_socket, SHUT_WR);

    // Discard all pending messages from the server and block until EOF.
    while (true) {
        uint8_t msg_buf[MAX_MSG_SIZE] = {0};
        // POSIX says we should never get SIGPIPE except on writes,
        // but set MSG_NOSIGNAL just in case.
        ssize_t bytes_read = recv(s_session_socket, msg_buf, sizeof(msg_buf), MSG_NOSIGNAL);
        if (bytes_read == -1) {
            throw_system_error("read failed");
        } else if (bytes_read == 0) {
            break;
        }
    }

    close(s_session_socket);
    s_session_socket = -1;
}

void client::begin_transaction()
{
    verify_session_active();
    verify_no_tx();

    // First we allocate a new log segment and map it in our own process.
    s_fd_log = memfd_create(SCH_MEM_LOG, MFD_ALLOW_SEALING);
    if (s_fd_log == -1)
    {
        throw_system_error("memfd_create failed");
    }
    if (-1 == ftruncate(s_fd_log, sizeof(log)))
    {
        throw_system_error("ftruncate failed");
    }
    s_log = static_cast<log*>(map_fd(sizeof(log), PROT_READ | PROT_WRITE, MAP_SHARED, s_fd_log, 0));

    // Now we map a private COW view of the locator shared memory segment.
    if (flock(s_fd_offsets, LOCK_SH) < 0)
    {
        throw_system_error("flock failed");
    }
    auto cleanup = scope_guard::make_scope_guard([]() {
        if (-1 == flock(s_fd_offsets, LOCK_UN))
        {
            // Per C++11 semantics, throwing an exception from a destructor
            // will just call std::terminate(), no undefined behavior.
            throw_system_error("flock failed");
        }
    });
    s_offsets = static_cast<offsets*>(map_fd(sizeof(offsets),
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_POPULATE, s_fd_offsets, 0));

    // Notify the server that we're in a transaction. (We don't send our log fd until commit.)
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::BEGIN_TXN);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Don't call the hook unless all preceding steps were successful.
    if (s_tx_begin_hook) {
        (s_tx_begin_hook.load())();
    }
}

void client::rollback_transaction()
{
    verify_tx_active();

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = scope_guard::make_scope_guard(tx_cleanup);

    // Notify the server that we rolled back this transaction.
    // (We don't expect the server to reply to this message.)
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::ROLLBACK_TXN);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Don't call the hook unless all preceding steps were successful.
    if (s_tx_rollback_hook) {
        (s_tx_rollback_hook.load())();
    }
}

// This method returns true for a commit decision and false for an abort decision.
// It sends a message to the server containing the fd of this txn's log segment and
// will block waiting for a reply from the server.
bool client::commit_transaction()
{
    verify_tx_active();

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = scope_guard::make_scope_guard(tx_cleanup);

    // Unmap the log segment so we can seal it.
    destroy_log_mapping();
    // Seal the txn log memfd for writes before sending it to the server.
    if (-1 == fcntl(s_fd_log, F_ADD_SEALS, F_SEAL_WRITE))
    {
        throw_system_error("fcntl(F_SEAL_WRITE) failed");
    }

    // Send the server the commit event with the log segment fd.
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::COMMIT_TXN);
    send_msg_with_fds(s_session_socket, &s_fd_log, 1, builder.GetBufferPointer(), builder.GetSize());

    // Block on the server's commit decision.
    uint8_t msg_buf[MAX_MSG_SIZE] = {0};
    size_t bytes_read = recv_msg_with_fds(s_session_socket, nullptr, nullptr, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0);

    // Extract the commit decision from the server's reply and return it.
    const message_t *msg = Getmessage_t(msg_buf);
    const server_reply_t *reply = msg->msg_as_reply();
    const session_event_t event = reply->event();
    retail_assert(event == session_event_t::DECIDE_TXN_COMMIT || event == session_event_t::DECIDE_TXN_ABORT);

    // Don't call the hook unless all preceding steps were successful.
    if (s_tx_commit_hook) {
        (s_tx_commit_hook.load())();
    }

    return (event == session_event_t::DECIDE_TXN_COMMIT);
}
