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
#include <thread>
#include <unordered_set>

#include <flatbuffers/flatbuffers.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_shared_data.hpp"
#include "db_types.hpp"
#include "fd_helpers.hpp"
#include "generator_iterator.hpp"
#include "memory_allocation_error.hpp"
#include "messages_generated.h"
#include "mmap_helpers.hpp"
#include "retail_assert.hpp"
#include "scope_guard.hpp"
#include "socket_helpers.hpp"
#include "system_error.hpp"
#include "triggers.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::db::messages;
using namespace gaia::db::memory_manager;
using namespace flatbuffers;
using namespace scope_guard;

int client::get_id_cursor_socket_for_type(gaia_type_t type)
{
    // Send the server the cursor socket request.
    FlatBufferBuilder builder;
    auto table_scan_info = Createtable_scan_info_t(builder, type);
    auto client_request = Createclient_request_t(builder, session_event_t::REQUEST_STREAM, request_data_t::table_scan, table_scan_info.Union());
    auto message = Createmessage_t(builder, any_message_t::request, client_request.Union());
    builder.Finish(message);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Extract the stream socket fd from the server's response.
    uint8_t msg_buf[c_max_msg_size] = {0};
    int stream_socket = -1;
    size_t fd_count = 1;
    size_t bytes_read = recv_msg_with_fds(s_session_socket, &stream_socket, &fd_count, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0, "Failed to read message!");
    retail_assert(fd_count == 1, "Unexpected fd count!");
    retail_assert(stream_socket != -1, "Invalid stream socket!");
    auto cleanup_stream_socket = make_scope_guard([&]() {
        close_fd(stream_socket);
    });

    // Deserialize the server message.
    const message_t* msg = Getmessage_t(msg_buf);
    const server_reply_t* reply = msg->msg_as_reply();
    const session_event_t event = reply->event();
    retail_assert(event == session_event_t::REQUEST_STREAM, "Unexpected event received!");

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
        retail_assert(stream_socket != -1, "Stream socket is invalid!");

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
                throw_system_error("recv(MSG_PEEK) failed");
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
            retail_assert(datagram_size % sizeof(T_element_type) == 0, "Unexpected datagram size!");

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
                throw_system_error("recv failed");
            }

            // Since our buffer is exactly the same size as the datagram,
            // we should read exactly the number of bytes in the datagram.
            retail_assert(bytes_read == datagram_size, "Bytes read differ from datagram size!");
        }

        // At this point we know our buffer is non-empty.
        retail_assert(batch_buffer.size() > 0, "Empty batch buffer detected!");

        // Loop through the buffer and return entries in FIFO order
        // (the server reversed the original buffer before sending).
        T_element_type next_value = batch_buffer.back();
        batch_buffer.pop_back();
        return next_value;
    };
}

// This generator wraps a socket which reads a stream of fds from the server.
// Because the socket interfaces for file descriptor passing are so specialized,
// we can't reuse the generic stream socket generator implementation.
std::function<std::optional<int>()>
client::get_fd_stream_generator_for_socket(int stream_socket)
{
    // Verify that the socket is the correct type for the semantics we assume.
    check_socket_type(stream_socket, SOCK_SEQPACKET);

    // The userspace buffer that we use to receive a batch datagram message.
    std::vector<int> batch_buffer;

    // The definition of the generator we return.
    return [stream_socket, batch_buffer]() mutable -> std::optional<int> {
        // We shouldn't be called again after we received EOF from the server.
        retail_assert(stream_socket != -1, "Stream socket is invalid!");

        // If buffer is empty, block until a new batch is available.
        if (batch_buffer.size() == 0)
        {
            // We only need a 1-byte buffer due to our datagram size convention.
            uint8_t msg_buf[1] = {0};

            // Resize the vector for maximum fd count, since we don't know
            // beforehand how many we'll get.
            batch_buffer.resize(c_max_fd_count);
            size_t fd_count = batch_buffer.size();

            // We need to set throw_on_zero_bytes_read=false in this call, since
            // we expect EOF to be returned when the stream is exhausted.
            size_t datagram_size = recv_msg_with_fds(stream_socket, batch_buffer.data(), &fd_count, msg_buf, sizeof(msg_buf), false);

            // Our datagrams contain one byte by convention, since we need to
            // distinguish empty datagrams from EOF (even though Linux allows
            // ancillary data with empty datagrams).
            if (datagram_size == 0)
            {
                // We received EOF from the server, so close
                // client socket and stop iteration.
                close_fd(stream_socket);

                // Tell the caller to stop iteration.
                return std::nullopt;
            }

            // The datagram size must be 1 byte according to our protocol.
            retail_assert(datagram_size == 1, "Unexpected datagram size!");
            retail_assert(
                fd_count > 0 && fd_count <= c_max_fd_count,
                "Unexpected fd count!");

            // Resize the vector to its actual fd count.
            batch_buffer.resize(fd_count);
        }

        // At this point we know our buffer is non-empty.
        retail_assert(batch_buffer.size() > 0, "Empty batch buffer detected!");

        // Loop through the buffer and return entries in FIFO order
        // (the server reversed the original buffer before sending).
        int next_fd = batch_buffer.back();
        batch_buffer.pop_back();
        return next_fd;
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

// Coalesces multiple updates to a single locator into the last update. In the
// future, some intermediate update log records can be eliminated by updating
// objects in-place, but not all of them (e.g., any size-changing updates to
// variable-size fields must copy the whole object first).
//
// REVIEW: Currently we leak all object versions from "duplicate" log records
// removed by this method! We need to pass a list of the offsets of removed
// versions to the memory manager for deallocation, possibly in a separate
// shared memory object.
void client::dedup_log()
{
    retail_assert(s_log, "Transaction log must be mapped!");

    // First sort the record array so we can call std::unique() on it. We use
    // stable_sort() to preserve the order of multiple updates to the same
    // locator.
    std::stable_sort(
        &s_log->log_records[0],
        &s_log->log_records[s_log->count],
        [](const txn_log_t::log_record_t& lhs, const txn_log_t::log_record_t& rhs) {
            return lhs.locator < rhs.locator;
        });

    // This is a bit weird: we want to preserve the *last* update to a locator,
    // but std::unique preserves the *first* duplicate it encounters.
    // So we reverse the sorted array so that the last update will be the first
    // that std::unique sees, then reverse the deduplicated array so that locators
    // are again in ascending order.
    std::reverse(&s_log->log_records[0], &s_log->log_records[s_log->count]);

    // More weirdness: we need to record the initial offset of the first log
    // record for each locator, so we can fix up the initial offset of its last
    // log record (the one we will keep) with this offset. Otherwise we'll get
    // bogus write conflicts because the initial offset of the last log record
    // for a locator doesn't match its last known offset in the locator segment.
    // Since the array is reversed, we'll see the first log record for a locator
    // last, so we get the desired behavior if we overwrite the last seen entry
    // in the initial offsets map.
    std::unordered_map<gaia_locator_t, gaia_offset_t> initial_offsets;

    for (size_t i = 0; i < s_log->count; ++i)
    {
        gaia_locator_t locator = s_log->log_records[i].locator;
        gaia_offset_t initial_offset = s_log->log_records[i].old_offset;
        initial_offsets[locator] = initial_offset;
    }

    txn_log_t::log_record_t* unique_array_end = std::unique(
        &s_log->log_records[0],
        &s_log->log_records[s_log->count],
        [](const txn_log_t::log_record_t& lhs, const txn_log_t::log_record_t& rhs) -> bool {
            return lhs.locator == rhs.locator;
        });

    // It's OK to leave the duplicate log records at the end of the array, since
    // they'll be removed when we truncate the txn log memfd before sending it
    // to the server.
    size_t unique_array_len = unique_array_end - s_log->log_records;
    s_log->count = unique_array_len;

    // Now that all log records for each locator are deduplicated, reverse the
    // array again to order by locator value in ascending order.
    std::reverse(&s_log->log_records[0], &s_log->log_records[s_log->count]);

    // Now we need to fix up each log record with the initial offset we recorded earlier.
    retail_assert(
        s_log->count == initial_offsets.size(),
        "Count of deduped log records must equal number of unique initial offsets!");

    for (size_t i = 0; i < s_log->count; ++i)
    {
        gaia_locator_t locator = s_log->log_records[i].locator;
        gaia_offset_t initial_offset = initial_offsets[locator];
        s_log->log_records[i].old_offset = initial_offset;
    }
}

// This function must be called before establishing a new session. It ensures
// that if the server restarts or is reset, no session will start with a stale
// data mapping or locator fd.
void client::clear_shared_memory()
{
    // This is intended to be called before a session is established.
    verify_no_session();

    // We closed our original fd for the data segment, so we only need to unmap it.
    unmap_fd(s_data, sizeof(*s_data));

    // If the server has already closed its fd for the locator segment
    // (and there are no other clients), this will release it.
    close_fd(s_fd_locators);
}

void client::txn_cleanup()
{
    // Destroy the log memory mapping.
    unmap_fd(s_log, c_initial_log_size);

    // Destroy the log fd.
    close_fd(s_fd_log);

    // Destroy the locator mapping.
    unmap_fd(s_locators, sizeof(*s_locators));
}

int client::get_session_socket()
{
    // Unlike the session socket on the server, this socket must be blocking,
    // since we don't read within a multiplexing poll loop.
    int session_socket = ::socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (session_socket == -1)
    {
        throw_system_error("socket creation failed");
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
void client::begin_session()
{
    // Fail if a session already exists on this thread.
    verify_no_session();

    // Clean up possible stale state from a server crash or reset.
    clear_shared_memory();

    // Assert relevant fd's and pointers are in clean state.
    retail_assert(s_data == nullptr, "Data segment uninitialized");
    retail_assert(s_fd_locators == -1, "Locators file descriptor uninitialized");
    retail_assert(s_fd_log == -1, "Log file descriptor uninitialized");
    retail_assert(s_log == nullptr, "Log segment uninitialized");
    retail_assert(s_locators == nullptr, "Locators segment uninitialized");

    // Connect to the server's well-known socket name, and ask it
    // for the data and locator shared memory segment fds.
    s_session_socket = get_session_socket();

    auto cleanup_session_socket = make_scope_guard([&]() {
        close_fd(s_session_socket);
    });

    // Send the server the connection request.
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::CONNECT);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Extract the data and locator shared memory segment fds from the server's response.
    uint8_t msg_buf[c_max_msg_size] = {0};
    constexpr size_t c_fd_count = 4;
    int fds[c_fd_count] = {-1};
    size_t fd_count = c_fd_count;
    size_t bytes_read = recv_msg_with_fds(s_session_socket, fds, &fd_count, msg_buf, sizeof(msg_buf));
    auto [fd_locators, fd_counters, fd_data, fd_id_index] = fds;
    retail_assert(bytes_read > 0, "Failed to read message!");
    retail_assert(fd_count == c_fd_count, "Unexpected fd count!");
    retail_assert(fd_locators != -1, "Invalid locators fd detected!");
    retail_assert(fd_counters != -1, "Invalid counters fd detected!");
    retail_assert(fd_data != -1, "Invalid data fd detected!");
    retail_assert(fd_id_index != -1, "Invalid id_index fd detected!");

    // We need to use the initializer + mutable hack to capture structured bindings in a lambda.
    auto cleanup_mapping_fds = make_scope_guard([fd_counters = fd_counters,
                                                 fd_data = fd_data,
                                                 fd_id_index = fd_id_index]() mutable {
        // We can unconditionally close all the fds of shared mappings, since
        // they're not saved anywhere and each mapping increments its shared
        // memory segment's refcount.
        close_fd(fd_counters);
        close_fd(fd_data);
        close_fd(fd_id_index);
    });

    // We need to use the initializer + mutable hack to capture structured bindings in a lambda.
    auto cleanup_locator_fd = make_scope_guard([fd_locators = fd_locators]() mutable {
        // We should only close the locator fd if we are unwinding from an
        // exception, since we cache it to map later.
        close_fd(fd_locators);
    });

    const message_t* msg = Getmessage_t(msg_buf);
    const server_reply_t* reply = msg->msg_as_reply();
    session_event_t event = reply->event();
    retail_assert(event == session_event_t::CONNECT, "Unexpected event received!");

    // Set up the shared-memory mappings (see notes in db_server.cpp).
    map_fd(s_counters, sizeof(*s_counters), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd_counters, 0);
    map_fd(s_data, sizeof(*s_data), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd_data, 0);
    map_fd(s_id_index, sizeof(*s_id_index), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, fd_id_index, 0);

    // Set up the private locator segment fd.
    s_fd_locators = fd_locators;
    cleanup_locator_fd.dismiss();
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
    retail_assert(!s_locators, "Locators segment should be uninitialized!");
    map_fd(s_locators, sizeof(*s_locators), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_NORESERVE, s_fd_locators, 0);
    auto cleanup_locator_mapping = make_scope_guard([&]() {
        unmap_fd(s_locators, sizeof(*s_locators));
    });

    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::BEGIN_TXN);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Block to receive transaction id and txn logs from the server.
    uint8_t msg_buf[c_max_msg_size] = {0};
    int stream_socket = -1;
    size_t fd_count = 1;
    size_t bytes_read = recv_msg_with_fds(s_session_socket, &stream_socket, &fd_count, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0, "Failed to read message!");
    retail_assert(fd_count == 1, "Unexpected fd count!");
    retail_assert(stream_socket != -1, "Invalid stream socket!");
    auto cleanup_stream_socket = make_scope_guard([&]() {
        close_fd(stream_socket);
    });

    // Extract the transaction id and cache it; it needs to be reset for the next transaction.
    const message_t* msg = Getmessage_t(msg_buf);
    const server_reply_t* reply = msg->msg_as_reply();
    const transaction_info_t* txn_info = reply->data_as_transaction_info();
    s_txn_id = txn_info->transaction_id();
    retail_assert(
        s_txn_id != c_invalid_gaia_txn_id,
        "Begin timestamp should not be invalid!");

    // Allocate a new log segment and map it in our own process.
    std::stringstream memfd_name;
    memfd_name << c_sch_mem_txn_log << ':' << s_txn_id;
    int fd_log = ::memfd_create(memfd_name.str().c_str(), MFD_ALLOW_SEALING);
    if (fd_log == -1)
    {
        throw_system_error("memfd_create failed");
    }
    auto cleanup_log_fd = make_scope_guard([&]() {
        close_fd(fd_log);
    });
    truncate_fd(fd_log, c_initial_log_size);
    map_fd(s_log, c_initial_log_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_log, 0);
    auto cleanup_log_mapping = make_scope_guard([&]() {
        unmap_fd(s_log, c_initial_log_size);
    });

    // Update the log header with our begin timestamp.
    s_log->begin_ts = s_txn_id;

    // Apply all txn logs received from server to our snapshot, in order. The
    // generator will close the stream socket when it's exhausted, but we need
    // to close the stream socket if the generator throws an exception.
    // REVIEW: might need to think about clearer ownership of the stream socket.
    auto fd_generator = get_fd_stream_generator_for_socket(stream_socket);
    auto txn_log_fds = gaia::common::iterators::range_from_generator(fd_generator);
    for (int txn_log_fd : txn_log_fds)
    {
        apply_txn_log(txn_log_fd);
        close_fd(txn_log_fd);
    }

    // Save the log fd to send to the server on commit.
    s_fd_log = fd_log;

    // If we exhausted the generator without throwing an exception, then the
    // generator already closed the stream socket.
    cleanup_stream_socket.dismiss();
    cleanup_locator_mapping.dismiss();
    cleanup_log_mapping.dismiss();
    cleanup_log_fd.dismiss();
}

void client::apply_txn_log(int log_fd)
{
    retail_assert(s_locators, "Locators segment must be mapped!");
    txn_log_t* txn_log;
    map_fd(txn_log, get_fd_size(log_fd), PROT_READ, MAP_PRIVATE, log_fd, 0);
    auto cleanup_log_mapping = make_scope_guard([&]() {
        unmap_fd(txn_log, get_fd_size(log_fd));
    });
    for (size_t i = 0; i < txn_log->count; ++i)
    {
        auto lr = txn_log->log_records + i;
        (*s_locators)[lr->locator] = lr->new_offset;
    }
}

void client::rollback_transaction()
{
    verify_txn_active();

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = make_scope_guard(txn_cleanup);

    size_t log_size = s_log->size();
    unmap_fd(s_log, c_initial_log_size);
    truncate_fd(s_fd_log, log_size);

    // Seal the txn log memfd for writes/resizing before sending it to the server.
    if (-1 == ::fcntl(s_fd_log, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE))
    {
        throw_system_error("fcntl(F_ADD_SEALS) failed");
    }

    // No allocations made.
    if (log_size > 0)
    {
        FlatBufferBuilder builder;
        build_client_request(builder, session_event_t::ROLLBACK_TXN);
        send_msg_with_fds(s_session_socket, &s_fd_log, 1, builder.GetBufferPointer(), builder.GetSize());

        std::cout << "waiting on rollback for xid" << s_txn_id << std::endl;
        // Block on server side cleanup of transaction log.
        uint8_t msg_buf[c_max_msg_size] = {0};
        size_t bytes_read = recv_msg_with_fds(s_session_socket, nullptr, nullptr, msg_buf, sizeof(msg_buf));
        retail_assert(bytes_read > 0, "Failed to read message!");
        std::cout << "done rollback for xid" << s_txn_id << std::endl;
    }

    // Reset transaction id.
    s_txn_id = c_invalid_gaia_txn_id;

    // Reset TLS events vector for the next transaction that will run on this thread.
    s_events.clear();
}

// This method returns void on a commit decision and throws on an abort decision.
// It sends a message to the server containing the fd of this txn's log segment and
// will block waiting for a reply from the server.
void client::commit_transaction()
{
    verify_txn_active();
    retail_assert(s_log, "Transaction log must be mapped!");

    // This optimization to treat committing a read-only txn as a rollback
    // allows us to avoid any special cases in the server for empty txn logs.
    if (s_log->count == 0)
    {
        rollback_transaction();
        return;
    }

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = make_scope_guard(txn_cleanup);

    // Remove intermediate update log records.
    // FIXME: this leaks all intermediate object versions!!!
    dedup_log();

    // Get final size of log.
    size_t log_size = s_log->size();

    // Unmap the log segment so we can truncate and seal it.
    unmap_fd(s_log, c_initial_log_size);

    // Truncate the log segment to its final size.
    truncate_fd(s_fd_log, log_size);

    // Seal the txn log memfd for writes/resizing before sending it to the server.
    if (-1 == ::fcntl(s_fd_log, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE))
    {
        throw_system_error("fcntl(F_ADD_SEALS) failed");
    }

    // Send the server the commit event with the log segment fd.
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::COMMIT_TXN);
    send_msg_with_fds(s_session_socket, &s_fd_log, 1, builder.GetBufferPointer(), builder.GetSize());

    // Block on the server's commit decision.
    uint8_t msg_buf[c_max_msg_size] = {0};
    size_t bytes_read = recv_msg_with_fds(s_session_socket, nullptr, nullptr, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0, "Failed to read message!");

    // Extract the commit decision from the server's reply and return it.
    const message_t* msg = Getmessage_t(msg_buf);
    const server_reply_t* reply = msg->msg_as_reply();
    session_event_t event = reply->event();
    retail_assert(
        event == session_event_t::DECIDE_TXN_COMMIT || event == session_event_t::DECIDE_TXN_ABORT,
        "Unexpected event received!");
    const transaction_info_t* txn_info = reply->data_as_transaction_info();
    retail_assert(txn_info->transaction_id() == s_txn_id, "Unexpected transaction id!");

    // Throw an exception on server-side abort.
    // REVIEW: We could include the gaia_ids of conflicting objects in
    // transaction_update_conflict
    // (https://gaiaplatform.atlassian.net/browse/GAIAPLAT-292).
    if (event == session_event_t::DECIDE_TXN_ABORT)
    {
        throw transaction_update_conflict();
    }

    // Execute trigger only if rules engine is initialized.
    if (s_txn_commit_trigger
        && event == session_event_t::DECIDE_TXN_COMMIT
        && s_events.size() > 0)
    {
        s_txn_commit_trigger(s_txn_id, s_events);
    }
    // Reset transaction id.
    s_txn_id = c_invalid_gaia_txn_id;

    // Reset TLS events vector for the next transaction that will run on this thread.
    s_events.clear();
}

address_offset_t client::request_memory(size_t object_size)
{
    verify_txn_active();

    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::REQUEST_MEMORY, object_size);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Receive memory allocation information from the server.
    uint8_t msg_buf[c_max_msg_size] = {0};
    size_t bytes_read = recv_msg_with_fds(s_session_socket, nullptr, nullptr, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0, "Failed to read message!");

    const message_t* msg = Getmessage_t(msg_buf);
    const server_reply_t* reply = msg->msg_as_reply();
    const memory_allocation_info_t* allocation_info = reply->data_as_memory_allocation_info();

    // Obtain allocated offset from the server.
    const address_offset_t object_address_offset = allocation_info->allocation_offset();

    retail_assert(allocation_info && object_address_offset != c_invalid_offset, "Failed to fetch memory from the server.");

    return object_address_offset;
}

address_offset_t client::allocate_object(
    gaia_locator_t locator,
    address_offset_t old_slot_offset,
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
