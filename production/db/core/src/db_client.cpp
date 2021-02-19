/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_client.hpp"

#include <unistd.h>

#include <csignal>

#include <atomic>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>

#include <flatbuffers/flatbuffers.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/memory_allocation_error.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/common/socket_helpers.hpp"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/triggers.hpp"

#include "client_messenger.hpp"
#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_shared_data.hpp"
#include "messages_generated.h"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::db::messages;
using namespace gaia::db::memory_manager;
using namespace flatbuffers;
using namespace scope_guard;

static const std::string c_message_unexpected_event_received = "Unexpected event received!";
static const std::string c_message_stream_socket_is_invalid = "Stream socket is invalid!";
static const std::string c_message_unexpected_datagram_size = "Unexpected datagram size!";
static const std::string c_message_empty_batch_buffer_detected = "Empty batch buffer detected!";

int client::get_id_cursor_socket_for_type(gaia_type_t type)
{
    // Build the cursor socket request.
    FlatBufferBuilder builder;
    auto table_scan_info = Createtable_scan_info_t(builder, type);
    auto client_request = Createclient_request_t(builder, session_event_t::REQUEST_STREAM, request_data_t::table_scan, table_scan_info.Union());
    auto message = Createmessage_t(builder, any_message_t::request, client_request.Union());
    builder.Finish(message);

    client_messenger_t client_messenger(1);
    client_messenger.send_and_receive(s_session_socket, nullptr, 0, builder);

    int stream_socket = client_messenger.get_received_fd(0);
    auto cleanup_stream_socket = make_scope_guard([&]() {
        close_fd(stream_socket);
    });

    const session_event_t event = client_messenger.get_server_reply()->event();
    retail_assert(event == session_event_t::REQUEST_STREAM, c_message_unexpected_event_received);

    // Check that our stream socket is blocking (since we need to perform blocking reads).
    retail_assert(!is_non_blocking(stream_socket), "Stream socket is not set to blocking!");

    cleanup_stream_socket.dismiss();
    return stream_socket;
}

// This generator wraps a socket which reads a stream of values of `T_element_type` from the server.
template <typename T_element_type>
std::function<std::optional<T_element_type>()>
client::get_stream_generator_for_socket(int stream_socket)
{
    // Verify that the socket is the correct type for the semantics we assume.
    check_socket_type(stream_socket, SOCK_SEQPACKET);

    // Currently, we associate a cursor with a snapshot view, i.e., a transaction.
    verify_txn_active();
    gaia_txn_id_t owning_txn_id = s_txn_id;

    // The userspace buffer that we use to receive a batch datagram message.
    std::vector<T_element_type> batch_buffer;

    // The definition of the generator we return.
    return [stream_socket, owning_txn_id, batch_buffer]() mutable -> std::optional<T_element_type> {
        // We shouldn't be called again after we received EOF from the server.
        retail_assert(stream_socket != -1, c_message_stream_socket_is_invalid);

        // The cursor should only be called from within the scope of its owning transaction.
        retail_assert(s_txn_id == owning_txn_id, "Cursor was not called from the scope of its own transaction!");

        // If buffer is empty, block until a new batch is available.
        if (batch_buffer.size() == 0)
        {
            // Get the datagram size, and grow the buffer if necessary.
            // This decouples the client from the server (i.e., client
            // doesn't need to know server batch size), at the expense
            // of an extra system call per batch.
            // We set MSG_PEEK to avoid reading the datagram into our buffer,
            // and we set MSG_TRUNC to return the actual buffer size needed.
            ssize_t datagram_size = ::recv(stream_socket, nullptr, 0, MSG_PEEK | MSG_TRUNC);
            if (datagram_size == -1)
            {
                throw_system_error("recv(MSG_PEEK) failed!");
            }

            if (datagram_size == 0)
            {
                // We received EOF from the server, so close
                // client socket and stop iteration.
                close_fd(stream_socket);
                // Tell the caller to stop iteration.
                return std::nullopt;
            }

            // The datagram size must be an integer multiple of our datum size.
            retail_assert(datagram_size % sizeof(T_element_type) == 0, c_message_unexpected_datagram_size);

            // Align the end of the buffer to the datagram size.
            // Per the C++ standard, this will never reduce capacity.
            batch_buffer.resize(datagram_size);

            // Get the actual data.
            // This is a nonblocking read, since the previous blocking
            // read will not return until data is available.
            ssize_t bytes_read = ::recv(stream_socket, batch_buffer.data(), batch_buffer.size(), MSG_DONTWAIT);
            if (bytes_read == -1)
            {
                // Per above, we should never have to block here.
                retail_assert(errno != EAGAIN && errno != EWOULDBLOCK, "Unexpected errno value!");
                throw_system_error("recv() failed!");
            }

            // Since our buffer is exactly the same size as the datagram,
            // we should read exactly the number of bytes in the datagram.
            retail_assert(bytes_read == datagram_size, "Bytes read differ from datagram size!");
        }

        // At this point we know our buffer is non-empty.
        retail_assert(batch_buffer.size() > 0, c_message_empty_batch_buffer_detected);

        // Loop through the buffer and return entries in FIFO order
        // (the server reversed the original buffer before sending).
        T_element_type next_value = batch_buffer.back();
        batch_buffer.pop_back();
        return next_value;
    };
}

std::function<std::optional<gaia_id_t>()>
client::get_id_generator_for_type(gaia_type_t type)
{
    int stream_socket = get_id_cursor_socket_for_type(type);
    auto cleanup_stream_socket = make_scope_guard([&]() {
        close_fd(stream_socket);
    });
    auto id_generator = get_stream_generator_for_socket<gaia_id_t>(stream_socket);
    cleanup_stream_socket.dismiss();
    return id_generator;
}

static void build_client_request(
    FlatBufferBuilder& builder,
    session_event_t event,
    size_t memory_request_size_hint = 0)
{
    flatbuffers::Offset<client_request_t> client_request;
    if (event == session_event_t::REQUEST_MEMORY || event == session_event_t::BEGIN_TXN)
    {
        // Request more memory from the server in case session thread is running low.
        auto alloc_info = Creatememory_allocation_info_t(builder, 0, memory_request_size_hint);
        client_request = Createclient_request_t(builder, event, request_data_t::memory_info, alloc_info.Union());
    }
    else
    {
        client_request = Createclient_request_t(builder, event);
    }
    auto message = Createmessage_t(builder, any_message_t::request, client_request.Union());
    builder.Finish(message);
}

// Sort all txn log records, by locator as primary key, and by offset as
// secondary key. This enables us to use fast binary search and binary merge
// algorithms for conflict detection.
void client::sort_log()
{
    retail_assert(s_log.is_set(), "Transaction log must be mapped!");

    // We use stable_sort() to preserve the order of multiple updates to the
    // same locator.
    std::stable_sort(
        &s_log.data()->log_records[0],
        &s_log.data()->log_records[s_log.data()->record_count],
        [](const txn_log_t::log_record_t& lhs, const txn_log_t::log_record_t& rhs) {
            return lhs.locator < rhs.locator;
        });
}

// This function must be called before establishing a new session. It ensures
// that if the server restarts or is reset, no session will start with a stale
// data mapping or locator fd.
void client::clear_shared_memory()
{
    // This is intended to be called before a session is established.
    verify_no_session();

    // We closed our original fds for these data segments, so we only need to unmap them.
    s_shared_counters.close();
    s_shared_data.close();
    s_shared_id_index.close();

    // If the server has already closed its fd for the locator segment
    // (and there are no other clients), this will release it.
    close_fd(s_fd_locators);
}

void client::txn_cleanup()
{
    // Destroy the log memory mapping.
    s_log.close();

    // Destroy the locator mapping.
    s_private_locators.close();

    // Reset transaction id.
    s_txn_id = c_invalid_gaia_txn_id;

    // Reset TLS events vector for the next transaction that will run on this thread.
    s_events.clear();
}

int client::get_session_socket()
{
    // Unlike the session socket on the server, this socket must be blocking,
    // since we don't read within a multiplexing poll loop.
    int session_socket = ::socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (session_socket == -1)
    {
        throw_system_error("Socket creation failed!");
    }

    auto cleanup_session_socket = make_scope_guard([&]() {
        close_fd(session_socket);
    });

    sockaddr_un server_addr = {0};
    server_addr.sun_family = AF_UNIX;

    // The socket name (minus its null terminator) needs to fit into the space
    // in the server address structure after the prefix null byte.
    retail_assert(strlen(c_db_server_socket_name) <= sizeof(server_addr.sun_path) - 1, "Socket name is too long!");

    // We prepend a null byte to the socket name so the address is in the
    // (Linux-exclusive) "abstract namespace", i.e., not bound to the
    // filesystem.
    ::strncpy(&server_addr.sun_path[1], c_db_server_socket_name, sizeof(server_addr.sun_path) - 1);

    // The socket name is not null-terminated in the address structure, but
    // we need to add an extra byte for the null byte prefix.
    socklen_t server_addr_size = sizeof(server_addr.sun_family) + 1 + strlen(&server_addr.sun_path[1]);
    if (-1 == ::connect(session_socket, reinterpret_cast<sockaddr*>(&server_addr), server_addr_size))
    {
        throw_system_error("Connect failed!");
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
void client::begin_session()
{
    // Fail if a session already exists on this thread.
    verify_no_session();

    // Clean up possible stale state from a server crash or reset.
    clear_shared_memory();

    // Assert relevant fd's and pointers are in clean state.
    retail_assert(!s_private_locators.is_set(), "Locators segment is already mapped!");
    retail_assert(!s_shared_counters.is_set(), "Counters segment is already mapped!");
    retail_assert(!s_shared_data.is_set(), "Data segment is already mapped!");
    retail_assert(!s_shared_id_index.is_set(), "ID index segment is already mapped!");

    retail_assert(!s_log.is_set(), "Log segment is already mapped!");

    // Connect to the server's well-known socket name, and ask it
    // for the data and locator shared memory segment fds.
    s_session_socket = get_session_socket();

    auto cleanup_session_socket = make_scope_guard([&]() {
        close_fd(s_session_socket);
    });

    // Send the server the connection request.
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::CONNECT);

    client_messenger_t client_messenger(4);
    client_messenger.send_and_receive(s_session_socket, nullptr, 0, builder);

    int fd_locators = client_messenger.get_received_fd(0);
    int fd_counters = client_messenger.get_received_fd(1);
    int fd_data = client_messenger.get_received_fd(2);
    int fd_id_index = client_messenger.get_received_fd(3);

    auto cleanup_fd_locators = make_scope_guard([&]() {
        close_fd(fd_locators);
    });
    auto cleanup_fd_counters = make_scope_guard([&]() {
        close_fd(fd_counters);
    });
    auto cleanup_fd_data = make_scope_guard([&]() {
        close_fd(fd_data);
    });
    auto cleanup_fd_id_index = make_scope_guard([&]() {
        close_fd(fd_id_index);
    });

    session_event_t event = client_messenger.get_server_reply()->event();
    retail_assert(event == session_event_t::CONNECT, c_message_unexpected_event_received);

    // Set up the shared-memory mappings (see notes in db_server.cpp).
    // The mapper objects will take ownership of the fds so we dismiss the guards first.
    cleanup_fd_counters.dismiss();
    s_shared_counters.open(fd_counters);
    cleanup_fd_data.dismiss();
    s_shared_data.open(fd_data);
    cleanup_fd_id_index.dismiss();
    s_shared_id_index.open(fd_id_index);

    // Set up the private locator segment fd.
    s_fd_locators = fd_locators;

    cleanup_fd_locators.dismiss();
    cleanup_session_socket.dismiss();
}

void client::end_session()
{
    // This will gracefully shut down the server-side session thread
    // and all other threads that session thread owns.
    close_fd(s_session_socket);
}

void client::begin_transaction()
{
    verify_session_active();
    verify_no_txn();

    // Map a private COW view of the locator shared memory segment.
    retail_assert(!s_private_locators.is_set(), "Locators segment is already mapped!");
    auto cleanup_private_locators = make_scope_guard([&]() {
        s_private_locators.close();
    });
    bool manage_fd = false;
    s_private_locators.open(s_fd_locators, manage_fd);

    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::BEGIN_TXN);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Block to receive transaction id and txn logs from the server.
    uint8_t msg_buf[c_max_msg_size] = {0};
    size_t bytes_read = recv_msg_with_fds(s_session_socket, nullptr, nullptr, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0, c_message_failed_to_read_message);

    // Extract the transaction id and cache it; it needs to be reset for the next transaction.
    const transaction_info_t* txn_info = client_messenger.get_server_reply()->data_as_transaction_info();
    s_txn_id = txn_info->transaction_id();
    retail_assert(
        s_txn_id != c_invalid_gaia_txn_id,
        "Begin timestamp should not be invalid!");

    // Allocate a new log segment and map it in our own process.
    std::stringstream mem_log_name;
    mem_log_name << c_gaia_mem_txn_log << ':' << s_txn_id;
    // Use a local variable to ensure cleanup in case of an error.
    mapped_log_t log;
    log.create(mem_log_name.str().c_str());

    // Update the log header with our begin timestamp.
    log.data()->begin_ts = s_txn_id;

    // Apply all txn logs received from the server to our snapshot, in order.
    size_t fds_remaining_count = txn_info->log_fd_count();
    while (fds_remaining_count > 0)
    {
        int fds[c_max_fd_count] = {-1};
        size_t fd_count = std::size(fds);
        size_t bytes_read = recv_msg_with_fds(s_session_socket, fds, &fd_count, msg_buf, sizeof(msg_buf));

        // The fds are attached to a dummy 1-byte datagram.
        retail_assert(bytes_read == 1, "Unexpected message size!");
        retail_assert(fd_count > 0, "Unexpected fd count!");

        // Apply log fds as we receive them, to avoid having to buffer all of them.
        for (size_t i = 0; i < fd_count; ++i)
        {
            int txn_log_fd = fds[i];
            apply_txn_log(txn_log_fd);
            close_fd(txn_log_fd);
        }

        fds_remaining_count -= fd_count;
    }

    // At this point, we can transfer ownership of local variables to static ones.
    s_log.reset(log);

    cleanup_private_locators.dismiss();
}

void client::apply_txn_log(int log_fd)
{
    retail_assert(s_private_locators.is_set(), "Locators segment must be mapped!");

    mapped_log_t txn_log;
    txn_log.open(log_fd);

    for (size_t i = 0; i < txn_log.data()->record_count; ++i)
    {
        auto lr = txn_log.data()->log_records + i;
        (*s_private_locators.data())[lr->locator] = lr->new_offset;
    }
}

void client::rollback_transaction()
{
    verify_txn_active();

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = make_scope_guard(txn_cleanup);

    int fd_log;
    size_t log_size;
    s_log.truncate_seal_and_close(fd_log, log_size);

    // We now own destruction of fd_log.
    auto cleanup_fd_log = make_scope_guard([&]() {
        close_fd(fd_log);
    });

    int* fds = nullptr;
    size_t fd_count = 0;

    // Avoid sending the log fd to the server for read-only transactions.
    if (log_size)
    {
        fds = &fd_log;
        fd_count = 1;
    }

    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::ROLLBACK_TXN);
    send_msg_with_fds(s_session_socket, fds, fd_count, builder.GetBufferPointer(), builder.GetSize());
}

// This method returns void on a commit decision and throws on an abort decision.
// It sends a message to the server containing the fd of this txn's log segment and
// will block waiting for a reply from the server.
void client::commit_transaction()
{
    verify_txn_active();
    retail_assert(s_log.is_set(), "Transaction log must be mapped!");

    // This optimization to treat committing a read-only txn as a rollback
    // allows us to avoid any special cases in the server for empty txn logs.
    if (s_log.data()->record_count == 0)
    {
        rollback_transaction();
        return;
    }

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = make_scope_guard(txn_cleanup);

    // Sort log by locator for fast conflict detection.
    sort_log();

    int fd_log;
    size_t log_size;
    s_log.truncate_seal_and_close(fd_log, log_size);

    // We now own destruction of fd_log.
    auto cleanup_fd_log = make_scope_guard([&]() {
        close_fd(fd_log);
    });

    // Send the server the commit event with the log segment fd.
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::COMMIT_TXN);

    client_messenger_t client_messenger;
    client_messenger.send_and_receive(s_session_socket, &fd_log, 1, builder);

    // Extract the commit decision from the server's reply and return it.
    session_event_t event = client_messenger.get_server_reply()->event();
    retail_assert(
        event == session_event_t::DECIDE_TXN_COMMIT || event == session_event_t::DECIDE_TXN_ABORT,
        c_message_unexpected_event_received);
    const transaction_info_t* txn_info = client_messenger.get_server_reply()->data_as_transaction_info();
    retail_assert(txn_info->transaction_id() == s_txn_id, "Unexpected transaction id!");

    // Execute trigger only if rules engine is initialized.
    if (s_txn_commit_trigger
        && event == session_event_t::DECIDE_TXN_COMMIT
        && s_events.size() > 0)
    {
        s_txn_commit_trigger(s_txn_id, s_events);
    }

    // Throw an exception on server-side abort.
    // REVIEW: We could include the gaia_ids of conflicting objects in
    // transaction_update_conflict
    // (https://gaiaplatform.atlassian.net/browse/GAIAPLAT-292).
    if (event == session_event_t::DECIDE_TXN_ABORT)
    {
        throw transaction_update_conflict();
    }
}

address_offset_t client::request_memory(size_t object_size)
{
    verify_txn_active();

    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::REQUEST_MEMORY, object_size);

    client_messenger_t client_messenger;
    client_messenger.send_and_receive(s_session_socket, nullptr, 0, builder);

    const memory_allocation_info_t* allocation_info
        = client_messenger.get_server_reply()->data_as_memory_allocation_info();

    // Obtain allocated offset from the server.
    const address_offset_t object_address_offset = allocation_info->allocation_offset();

    retail_assert(allocation_info && object_address_offset != c_invalid_offset, "Failed to fetch memory from the server.");

    return object_address_offset;
}

address_offset_t client::allocate_object(
    gaia_locator_t locator,
    size_t size)
{
    retail_assert(size != 0, "The client should not deallocate objects directly.");
    address_offset_t allocated_memory_offset = c_invalid_offset;

    allocated_memory_offset = client::request_memory(size);

    retail_assert(allocated_memory_offset != c_invalid_offset, "Allocation failure! Returned offset not initialized.");

    // Update locator array to point to the new offset.
    update_locator(locator, allocated_memory_offset);

    return allocated_memory_offset;
}
