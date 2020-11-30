/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "se_client.hpp"

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

#include "db_types.hpp"
#include "fd_helpers.hpp"
#include "generator_iterator.hpp"
#include "messages_generated.h"
#include "mmap_helpers.hpp"
#include "retail_assert.hpp"
#include "scope_guard.hpp"
#include "se_helpers.hpp"
#include "se_shared_data.hpp"
#include "se_types.hpp"
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
    int flags = ::fcntl(stream_socket, F_GETFL, 0);
    if (flags == -1)
    {
        throw_system_error("fcntl(F_GETFL) failed");
    }
    retail_assert(~(flags & O_NONBLOCK), "Stream socket is not set to blocking!");

    cleanup_stream_socket.dismiss();
    return stream_socket;
}

// This generator wraps a socket which reads a stream of values of `T_element_type` from the server.
template <typename T_element_type>
std::function<std::optional<T_element_type>()>
client::get_stream_generator_for_socket(int stream_socket)
{
    // Verify the socket is the correct type for the semantics we assume.
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

static std::vector<flatbuffers::Offset<stack_allocator_info_t>> build_stack_allocator_list(
    FlatBufferBuilder& builder,
    const std::vector<stack_allocator_t>& stack_allocators)
{
    std::vector<flatbuffers::Offset<stack_allocator_info_t>> memory_allocation;

    for (auto stack_allocator : stack_allocators)
    {
        const auto stack_allocator_info = Createstack_allocator_info_t(
            builder,
            stack_allocator.get_base_memory_offset(),
            stack_allocator.get_total_memory_size());

        memory_allocation.push_back(stack_allocator_info);
    }

    return memory_allocation;
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
        auto alloc_info = Creatememory_allocation_info_tDirect(builder, nullptr, memory_request_size_hint);
        client_request = Createclient_request_t(builder, event, request_data_t::memory_info, alloc_info.Union());
    }
    else
    {
        client_request = Createclient_request_t(builder, event);
    }
    auto message = Createmessage_t(builder, any_message_t::request, client_request.Union());
    builder.Finish(message);
}

void client::destroy_log_mapping()
{
    // We might have already destroyed the log mapping before committing the txn.
    if (s_log)
    {
        unmap_fd(s_log, sizeof(log));
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
    if (s_data)
    {
        unmap_fd(s_data, sizeof(data));
    }
    // If the server has already closed its fd for the locator segment
    // (and there are no other clients), this will release it.
    if (s_fd_locators != -1)
    {
        close_fd(s_fd_locators);
    }
}

void client::txn_cleanup()
{
    // Destroy the log memory mapping.
    destroy_log_mapping();
    // Destroy the log fd.
    close_fd(s_fd_log);
    // Destroy the locator mapping.
    if (s_locators)
    {
        unmap_fd(s_locators, sizeof(locators));
    }
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
    retail_assert(strlen(c_se_server_socket_name) <= sizeof(server_addr.sun_path) - 1, "Socket name is too long!");
    // We prepend a null byte to the socket name so the address is in the
    // (Linux-exclusive) "abstract namespace", i.e., not bound to the
    // filesystem.
    ::strncpy(&server_addr.sun_path[1], c_se_server_socket_name, sizeof(server_addr.sun_path) - 1);
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
    retail_assert(s_free_stack_allocators.empty(), "No memory should be reserved yet");

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
    constexpr size_t c_fd_count = 2;
    int fds[c_fd_count] = {-1};
    size_t fd_count = c_fd_count;
    constexpr size_t c_data_fd_index = 0;
    constexpr size_t c_locators_fd_index = 1;
    size_t bytes_read = recv_msg_with_fds(s_session_socket, fds, &fd_count, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0, "Failed to read message!");
    retail_assert(fd_count == c_fd_count, "Unexpected fd count!");
    int fd_data = fds[c_data_fd_index];
    retail_assert(fd_data != -1, "Invalid data fd detected!");
    auto cleanup_data_fd = make_scope_guard([&]() {
        // We can unconditionally close the data fd, since it's not saved
        // anywhere and the mapping increments the shared memory segment's
        // refcount.
        close_fd(fd_data);
    });
    int fd_locators = fds[c_locators_fd_index];
    retail_assert(fd_locators != -1, "Invalid locators fd detected!");
    auto cleanup_locator_fd = make_scope_guard([&]() {
        // We should only close the locator fd if we are unwinding from an
        // exception, since we cache it to map later.
        close_fd(fd_locators);
    });

    const message_t* msg = Getmessage_t(msg_buf);
    const server_reply_t* reply = msg->msg_as_reply();
    session_event_t event = reply->event();
    retail_assert(event == session_event_t::CONNECT, "Unexpected event received!");

    // Since the data and locator fds are global, we need to atomically update them
    // (but only if they're not already initialized).

    // Set up the shared data segment mapping.
    s_data = static_cast<data*>(map_fd(
        sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0));

    // Set up the private locator segment fd.
    s_fd_locators = fd_locators;
    cleanup_locator_fd.dismiss();
    cleanup_session_socket.dismiss();
}

void client::load_stack_allocators(const memory_allocation_info_t* allocation_info, uint8_t* data_mapping_base_addr)
{
    auto stack_allocators = allocation_info->stack_allocator_list();

    for (size_t i = 0; i < stack_allocators->size(); i++)
    {
        auto allocator = stack_allocators->Get(i);

        // Create and load stack allocator.
        stack_allocator_t stack_allocator;
        stack_allocator.load(data_mapping_base_addr, allocator->stack_allocator_offset(), allocator->stack_allocator_size());
        s_free_stack_allocators.push_back(stack_allocator);
    }
}

void client::end_session()
{
    // This will gracefully shut down the server-side session thread
    // and all other threads that session thread owns.
    close_fd(s_session_socket);

    // Clean up client side state.
    // The server will release memory for any uncommitted/unused stack allocators on the session thread.
    s_free_stack_allocators.clear();
}

void client::begin_transaction()
{
    verify_session_active();
    verify_no_txn();

    // First we allocate a new log segment and map it in our own process.
    int fd_log = ::memfd_create(c_sch_mem_log, MFD_ALLOW_SEALING);
    if (fd_log == -1)
    {
        throw_system_error("memfd_create failed");
    }
    auto cleanup_log_fd = make_scope_guard([&]() {
        close_fd(fd_log);
    });
    if (-1 == ::ftruncate(fd_log, sizeof(log)))
    {
        throw_system_error("ftruncate failed");
    }
    s_log = static_cast<log*>(map_fd(sizeof(log), PROT_READ | PROT_WRITE, MAP_SHARED, fd_log, 0));
    auto cleanup_log_mapping = make_scope_guard([&]() {
        unmap_fd(s_log, sizeof(log));
    });

    // Now we map a private COW view of the locator shared memory segment.
    if (::flock(s_fd_locators, LOCK_SH) < 0)
    {
        throw_system_error("flock failed");
    }
    auto cleanup_flock = make_scope_guard([&]() {
        if (-1 == ::flock(s_fd_locators, LOCK_UN))
        {
            // Per C++11 semantics, throwing an exception from a destructor
            // will just call std::terminate(), no undefined behavior.
            throw_system_error("flock failed");
        }
    });

    s_locators = static_cast<locators*>(map_fd(sizeof(locators), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_POPULATE, s_fd_locators, 0));
    auto cleanup_locator_mapping = make_scope_guard([&]() {
        unmap_fd(s_locators, sizeof(locators));
    });

    FlatBufferBuilder builder;
    bool need_more_memory = true;
    build_client_request(builder, session_event_t::BEGIN_TXN);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Block to receive transaction id from the server.
    uint8_t msg_buf[c_max_msg_size] = {0};
    size_t bytes_read = recv_msg_with_fds(s_session_socket, nullptr, nullptr, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0, "Failed to read message!");

    // Extract the transaction id and cache it; it needs to be reset for the next transaction.
    const message_t* msg = Getmessage_t(msg_buf);
    const server_reply_t* reply = msg->msg_as_reply();
    const transaction_info_t* txn_info = reply->data_as_transaction_info();
    s_txn_id = txn_info->transaction_id();
    // Save the log fd to send to the server on commit.
    s_fd_log = fd_log;

    // Obtain transaction memory allocation information.
    const memory_allocation_info_t* allocation_info = reply->session_memory_allocation_info();
    retail_assert(allocation_info && allocation_info->stack_allocator_list()->size() > 0, "Failed to fetch memory from the server.");
    load_stack_allocators(allocation_info, reinterpret_cast<uint8_t*>(s_data->objects));

    cleanup_log_fd.dismiss();
    cleanup_log_mapping.dismiss();
    cleanup_locator_mapping.dismiss();
}

void client::rollback_transaction()
{
    verify_txn_active();

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = make_scope_guard(txn_cleanup);

    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::ROLLBACK_TXN);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Reset transaction id.
    s_txn_id = 0;

    // Reset TLS events vector for the next transaction that will run on this thread.
    s_events.clear();
    // Reset stack allocator vector.
    s_free_stack_allocators.clear();
}

// This method returns void on a commit decision and throws on an abort decision.
// It sends a message to the server containing the fd of this txn's log segment and
// will block waiting for a reply from the server.
void client::commit_transaction()
{
    verify_txn_active();

    // Ensure we destroy the shared memory segment and memory mapping before we return.
    auto cleanup = make_scope_guard(txn_cleanup);

    // Unmap the log segment so we can seal it.
    destroy_log_mapping();
    // Seal the txn log memfd for writes before sending it to the server.
    if (-1 == ::fcntl(s_fd_log, F_ADD_SEALS, F_SEAL_WRITE))
    {
        throw_system_error("fcntl(F_SEAL_WRITE) failed");
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
    // REVIEW: We could include the gaia_ids of conflicting objects in transaction_update_conflict
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
    s_txn_id = 0;

    // Reset TLS events vector for the next transaction that will run on this thread.
    s_events.clear();
    // Reset stack allocator vector.
    s_free_stack_allocators.clear();
}

void client::request_memory()
{
    verify_txn_active();

    FlatBufferBuilder builder;
    txn_memory_request_size_hint_bytes = txn_memory_request_size_hint_bytes * memory_request_size_multiplier;
    build_client_request(builder, session_event_t::REQUEST_MEMORY, txn_memory_request_size_hint_bytes);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Receive memory allocation information from the server.
    uint8_t msg_buf[c_max_msg_size] = {0};
    size_t bytes_read = recv_msg_with_fds(s_session_socket, nullptr, nullptr, msg_buf, sizeof(msg_buf));
    retail_assert(bytes_read > 0, "Failed to read message!");

    const message_t* msg = Getmessage_t(msg_buf);
    const server_reply_t* reply = msg->msg_as_reply();
    const memory_allocation_info_t* allocation_info = reply->session_memory_allocation_info();

    retail_assert(allocation_info && allocation_info->stack_allocator_list()->size() > 0, "Failed to fetch memory from the server.");
    load_stack_allocators(allocation_info, reinterpret_cast<uint8_t*>(s_data->objects));
}

static address_offset_t get_stack_allocator_offset(
    gaia_locator_t locator,
    address_offset_t old_slot_offset,
    size_t size,
    std::vector<stack_allocator_t>& free_allocators)
{
    error_code_t result = error_code_t::not_set;
    address_offset_t allocated_memory_offset = c_invalid_offset;
    // We should only need two attempts to allocate memory.
    // If more than two attempts are required then it means the newly obtained free stack allocator
    // doesn't have enough memory for the current object - which should not happen.
    for (size_t allocation_attempt = 0; allocation_attempt < 2; allocation_attempt++)
    {
        // If free list is empty, make an IPC call to request more memory from the server.
        // Note that this call can crash the server in case the server runs out of memory.
        if (free_allocators.empty())
        {
            client::request_memory();
        }

        retail_assert(!free_allocators.empty(), "Unable to obtain memory from server.");

        // Try allocating object. If current stack allocator memory isn't enough, then fetch
        // another allocator from the free list.
        auto current_allocator = free_allocators.front();
        result = current_allocator.allocate(locator, old_slot_offset, size, allocated_memory_offset);

        // The stack allocator size may not be sufficient for the current object, so get a new one.
        if (result == error_code_t::insufficient_memory_size || result == error_code_t::memory_size_too_large)
        {
            free_allocators.erase(free_allocators.begin());
        }
        else
        {
            break;
        }
    }

    // Todo - add exception class.
    retail_assert(result == error_code_t::success, "Object Stack allocation failure!");
    // Size 0 indicates a deletion. We never expect memory to be allocated for a delete call.
    if (size == 0)
    {
        retail_assert(allocated_memory_offset == c_invalid_offset, "Memory allocated for a delete operation!");
    }
    else
    {
        retail_assert(allocated_memory_offset != c_invalid_offset, "Allocation failure! Stack allocator returned offset not initialized.");
    }

    return allocated_memory_offset;
}

void client::allocate_object(
    gaia_locator_t locator,
    address_offset_t old_slot_offset,
    size_t size)
{
    bool deallocate = size == 0;
    address_offset_t allocated_memory_offset = get_stack_allocator_offset(locator, old_slot_offset, size, client::s_free_stack_allocators);

    if (!deallocate)
    {
        retail_assert(allocated_memory_offset != c_invalid_offset, "offset should be valid!");
        // Update locator array to point to the new offset.
        update_locator(locator, allocated_memory_offset);
    }
}
