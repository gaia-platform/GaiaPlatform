/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine_client.hpp"
#include "system_table_types.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::db::messages;
using namespace flatbuffers;

thread_local se_base::offsets *client::s_offsets = nullptr;
thread_local int client::s_fd_log = -1;
thread_local int client::s_fd_offsets = -1;
thread_local se_base::data *client::s_data;
thread_local std::vector<trigger_event_t> client::s_events;
commit_trigger_fn gaia::db::s_tx_commit_trigger = nullptr;

std::unordered_set<gaia_type_t> client::trigger_excluded_types{
    static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_table),
    static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_field),
    static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_ruleset),
    static_cast<gaia_type_t>(system_table_type_t::catalog_gaia_rule),
    static_cast<gaia_type_t>(system_table_type_t::event_log),
    static_cast<gaia_type_t>(0)
};


static void build_client_request(FlatBufferBuilder &builder, session_event_t event) {
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

// This function is intended only for use in test scenarios, where we need to clear
// stale mappings and file descriptors referencing shared memory segments that have
// been reinitialized by the server following receipt of SIGHUP (test-only feature).
// It is not concurrency-safe, since we assume that no sessions are active while this
// function is being called, which is a reasonable assumption for tests.
void client::clear_shared_memory() {
    // This is intended to be called before any session is established.
    verify_no_session();
    // We closed our original fd for the data segment, so we only need to unmap it.
    if (s_data) {
        unmap_fd(s_data, sizeof(data));
        s_data = nullptr;
    }
    // If the server has already closed its fd for the offset segment
    // (and there are no other clients), this will release it.
    if (s_fd_offsets != -1) {
        close(s_fd_offsets);
        s_fd_offsets = -1;
    }
}

void client::tx_cleanup() {
    // Destroy the log memory mapping.
    destroy_log_mapping();
    // Destroy the log fd.
    close(s_fd_log);
    s_fd_log = -1;
    // Destroy the offset mapping.
    if (s_offsets) {
        unmap_fd(s_offsets, sizeof(offsets));
        s_offsets = nullptr;
    }
}

int client::get_session_socket() {
    // Unlike the session socket on the server, this socket must be blocking,
    // since we don't read within a multiplexing poll loop.
    int session_socket = socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (session_socket == -1) {
        throw_system_error("socket creation failed");
    }
    auto cleanup_session_socket = scope_guard::make_scope_guard([session_socket]() {
        close(session_socket);
    });
    struct sockaddr_un server_addr = {0};
    server_addr.sun_family = AF_UNIX;
    // The socket name (minus its null terminator) needs to fit into the space
    // in the server address structure after the prefix null byte.
    retail_assert(strlen(SE_SERVER_NAME) <= sizeof(server_addr.sun_path) - 1);
    // We prepend a null byte to the socket name so the address is in the
    // (Linux-exclusive) "abstract namespace", i.e., not bound to the
    // filesystem.
    strncpy(&server_addr.sun_path[1], SE_SERVER_NAME,
        sizeof(server_addr.sun_path) - 1);
    // The socket name is not null-terminated in the address structure, but
    // we need to add an extra byte for the null byte prefix.
    socklen_t server_addr_size =
        sizeof(server_addr.sun_family) + 1 + strlen(&server_addr.sun_path[1]);
    if (-1 == connect(session_socket, (struct sockaddr *)&server_addr, server_addr_size)) {
        throw_system_error("connect failed");
    }
    cleanup_session_socket.dismiss();
    return session_socket;
}

// This method establishes a session with the server on the calling thread,
// which serves as a transaction context in the client and a container for
// client state on the server. The server always passes fds for the data and
// locator shared memory segments, but we only use them if this is the client
// process's first call to create_session(), since they are stored globally
// rather than per-session. (The connected socket is stored per-session.)
// REVIEW: There is currently no way for the client to be asynchronously notified
// when the server closes the session (e.g., if the server process shuts down).
// Throwing an asynchronous exception on the session thread may not be possible,
// and would be difficult to handle properly even if it were possible.
// In any case, send_msg_with_fds()/recv_msg_with_fds() already throw a
// peer_disconnected exception when the other end of the socket is closed.
void client::begin_session() {
    // Fail if a session already exists on this thread.
    verify_no_session();
    // Clean up possible stale state from a server crash or reset.
    clear_shared_memory();
    // Assert relevant fd's and pointers are in clean state.
    retail_assert(s_data == nullptr, "Data segment uninitialized");
    retail_assert(s_fd_offsets == -1, "Offsets file descriptor uninitialized");
    retail_assert(s_fd_log == -1, "Log file descriptor uninitialized");
    retail_assert(s_log == nullptr, "Log uninitialized");
    retail_assert(s_offsets == nullptr, "Offset uninitialized");

    // Connect to the server's well-known socket name, and ask it
    // for the data and locator shared memory segment fds.
    s_session_socket = get_session_socket();

    auto cleanup_session_socket = scope_guard::make_scope_guard([]() {
        close(s_session_socket);
        s_session_socket = -1;
    });

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
    int fd_data = fds[DATA_FD_INDEX];
    retail_assert(fd_data != -1);
    int fd_offsets = fds[OFFSETS_FD_INDEX];
    retail_assert(fd_offsets != -1);
    auto cleanup_fds = scope_guard::make_scope_guard([fd_data, fd_offsets]() {
        // We can unconditionally close the data fd,
        // since it's not saved anywhere and the mapping
        // increments the shared memory segment's refcount.
        close(fd_data);
        // We can only close the locator fd if it hasn't been cached.
        if (s_fd_offsets != fd_offsets) {
            close(fd_offsets);
        }
    });

    const message_t *msg = Getmessage_t(msg_buf);
    const server_reply_t *reply = msg->msg_as_reply();
    const session_event_t event = reply->event();
    retail_assert(event == session_event_t::CONNECT);

    // Since the data and locator fds are global, we need to atomically update them
    // (but only if they're not already initialized).

    // Set up the shared data segment mapping.
    s_data = static_cast<data *>(map_fd(
        sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0));

    // We've already mapped the data fd, so we can close it now.
    close(fd_data);

    // Set up the private locator segment fd.
    s_fd_offsets = fd_offsets;
    cleanup_session_socket.dismiss();
}

void client::end_session() {
    // Send the server EOF.
    shutdown(s_session_socket, SHUT_WR);

    auto cleanup_session_socket = scope_guard::make_scope_guard([]() {
        close(s_session_socket);
        s_session_socket = -1;
    });

    // Discard all pending messages from the server and block until EOF.
    // REVIEW: Is there any reason not to just close the socket to begin with?
    // That *should* deliver EPOLLRDHUP to the other side (but needs testing).
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
}

void client::begin_transaction() {
    verify_session_active();
    verify_no_tx();

    // First we allocate a new log segment and map it in our own process.
    int fd_log = memfd_create(SCH_MEM_LOG, MFD_ALLOW_SEALING);
    if (fd_log == -1) {
        throw_system_error("memfd_create failed");
    }
    auto cleanup_fd = scope_guard::make_scope_guard([fd_log]() {
        close(fd_log);
    });
    if (-1 == ftruncate(fd_log, sizeof(log))) {
        throw_system_error("ftruncate failed");
    }
    s_log = static_cast<log *>(map_fd(sizeof(log), PROT_READ | PROT_WRITE, MAP_SHARED, fd_log, 0));
    auto cleanup_log_mapping = scope_guard::make_scope_guard([]() {
        unmap_fd(s_log, sizeof(log));
        s_log = nullptr;
    });

    // Now we map a private COW view of the locator shared memory segment.
    if (flock(s_fd_offsets, LOCK_SH) < 0) {
        throw_system_error("flock failed");
    }
    auto cleanup_flock = scope_guard::make_scope_guard([]() {
        if (-1 == flock(s_fd_offsets, LOCK_UN)) {
            // Per C++11 semantics, throwing an exception from a destructor
            // will just call std::terminate(), no undefined behavior.
            throw_system_error("flock failed");
        }
    });

    s_offsets = static_cast<offsets *>(map_fd(sizeof(offsets),
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_POPULATE, s_fd_offsets, 0));
    auto cleanup_locator_mapping = scope_guard::make_scope_guard([]() {
        unmap_fd(s_offsets, sizeof(offsets));
        s_offsets = nullptr;
    });

    // Notify the server that we're in a transaction. (We don't send our log fd until commit.)
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::BEGIN_TXN);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Block to receive transaction id from the server.
    uint8_t msg_buf[MAX_MSG_SIZE] = {0};
    size_t bytes_read = recv_msg_with_fds(s_session_socket, nullptr, nullptr, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0);

    // Extract the transaction id and cache it; it needs to be reset for the next transaction.
    const message_t *msg = Getmessage_t(msg_buf);
    const server_reply_t *reply = msg->msg_as_reply();
    s_transaction_id = reply->transaction_id();

    // Save the log fd to send to the server on commit.
    s_fd_log = fd_log;

    cleanup_fd.dismiss();
    cleanup_log_mapping.dismiss();
    cleanup_locator_mapping.dismiss();
}

void client::rollback_transaction() {
    verify_tx_active();

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = scope_guard::make_scope_guard(tx_cleanup);

    // Notify the server that we rolled back this transaction.
    // (We don't expect the server to reply to this message.)
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::ROLLBACK_TXN);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());
}

// This method returns void on a commit decision and throws on an abort decision.
// It sends a message to the server containing the fd of this txn's log segment and
// will block waiting for a reply from the server.
void client::commit_transaction() {
    verify_tx_active();

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = scope_guard::make_scope_guard(tx_cleanup);

    // Unmap the log segment so we can seal it.
    destroy_log_mapping();
    // Seal the txn log memfd for writes before sending it to the server.
    if (-1 == fcntl(s_fd_log, F_ADD_SEALS, F_SEAL_WRITE)) {
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

    // Throw an exception on server-side abort.
    // REVIEW: We could include the gaia_ids of conflicting objects in transaction_update_conflict
    // (https://gaiaplatform.atlassian.net/browse/GAIAPLAT-292).
    if (event == session_event_t::DECIDE_TXN_ABORT) {
        throw transaction_update_conflict();
    }

    // Execute trigger only if rules engine is initialized.
    if (s_tx_commit_trigger
        && event == session_event_t::DECIDE_TXN_COMMIT
        && s_events.size() > 0) {
        s_tx_commit_trigger(s_transaction_id, s_events);
    }
    // Reset transaction id.
    s_transaction_id = 0;

    // Reset TLS events vector for the next transaction that will run on this thread.
    s_events.clear();

}
