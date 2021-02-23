/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_server.hpp"

#include <unistd.h>

#include <csignal>

#include <algorithm>
#include <atomic>
#include <bitset>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <shared_mutex>
#include <thread>
#include <unordered_set>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/file.h>

#include "gaia_internal/common/fd_helpers.hpp"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/memory_allocation_error.hpp"
#include "gaia_internal/common/mmap_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/common/socket_helpers.hpp"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

#include "db_hash_map.hpp"
#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "db_shared_data.hpp"
#include "messages_generated.h"
#include "persistent_store_manager.hpp"

using namespace std;

using namespace gaia::db;
using namespace gaia::db::messages;
using namespace gaia::db::memory_manager;
using namespace gaia::common::iterators;
using namespace gaia::common::scope_guard;

static constexpr char c_message_uninitialized_fd_log[] = "Uninitialized fd log!";
static constexpr char c_message_unexpected_event_received[] = "Unexpected event received!";
static constexpr char c_message_current_event_is_inconsistent_with_state_transition[]
    = "Current event is inconsistent with state transition!";
static constexpr char c_message_unexpected_request_data_type[] = "Unexpected request data type!";
static constexpr char c_message_memfd_create_failed[] = "memfd_create() failed!";
static constexpr char c_message_thread_must_be_joinable[] = "Thread must be joinable!";
static constexpr char c_message_epoll_create1_failed[] = "epoll_create1() failed!";
static constexpr char c_message_epoll_wait_failed[] = "epoll_wait() failed!";
static constexpr char c_message_epoll_ctl_failed[] = "epoll_ctl() failed!";
static constexpr char c_message_unexpected_event_type[] = "Unexpected event type!";
static constexpr char c_message_epollerr_flag_should_not_be_set[] = "EPOLLERR flag should not be set!";
static constexpr char c_message_unexpected_fd[] = "Unexpected fd!";
static constexpr char c_message_not_a_begin_timestamp[] = "Not a begin timestamp!";
static constexpr char c_message_not_a_commit_timestamp[] = "Not a commit timestamp!";
static constexpr char c_message_unknown_timestamp[] = "Unknown timestamp!";
static constexpr char c_message_unexpected_errno_value[] = "Unexpected errno value!";
static constexpr char c_message_txn_log_fd_cannot_be_invalidated[]
    = "A txn log fd can only be invalidated if its commit_ts has been validated!";
static constexpr char c_message_txn_log_fd_should_have_been_invalidated[]
    = "If the validating txn's log fd has been invalidated, then the watermark must have advanced past the commit_ts of the txn being tested, and invalidated its log fd!";
static constexpr char c_message_unexpected_commit_ts_value[]
    = "The commit_ts whose log fd was invalidated must belong to either the validating txn or the txn being tested!";
static constexpr char c_message_validating_txn_should_have_been_validated[]
    = "The txn being tested can only have its log fd invalidated if the validating txn was validated!";
static constexpr char c_message_preceding_txn_should_have_been_validated[]
    = "A transaction with commit timestamp preceding this transaction's begin timestamp is undecided!";

address_offset_t server::allocate_from_memory_manager(
    size_t memory_request_size_bytes)
{
    retail_assert(
        memory_request_size_bytes > 0,
        "Requested allocation size for the allocate API should be greater than 0!");

    // Offset gets assigned; no need to set it.
    address_offset_t object_address_offset = s_memory_manager->allocate(memory_request_size_bytes);
    if (object_address_offset == c_invalid_offset)
    {
        throw memory_allocation_error("Memory manager ran out of memory during allocate() call!");
    }

    return object_address_offset;
}

// This assignment is non-atomic since there seems to be no reason to expect concurrent invocations.
void server::register_object_deallocator(std::function<void(gaia_offset_t)> deallocator_fn)
{
    s_object_deallocator_fn = deallocator_fn;
}

void server::handle_connect(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::CONNECT, c_message_unexpected_event_received);

    // This message should only be received after the client thread was first initialized.
    retail_assert(
        old_state == session_state_t::DISCONNECTED && new_state == session_state_t::CONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    // We need to reply to the client with the fds for the data/locator segments.
    FlatBufferBuilder builder;
    build_server_reply(builder, session_event_t::CONNECT, old_state, new_state);

    int send_fds[] = {s_fd_locators, s_fd_counters, s_fd_data, s_fd_id_index};
    send_msg_with_fds(s_session_socket, send_fds, std::size(send_fds), builder.GetBufferPointer(), builder.GetSize());
}

void server::handle_begin_txn(
    int*, size_t, session_event_t event, const void* event_data, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::BEGIN_TXN, c_message_unexpected_event_received);

    // This message should only be received while a transaction is in progress.
    retail_assert(
        old_state == session_state_t::CONNECTED && new_state == session_state_t::TXN_IN_PROGRESS,
        c_message_current_event_is_inconsistent_with_state_transition);

    auto request = static_cast<const client_request_t*>(event_data);
    retail_assert(
        request->data_type() == request_data_t::memory_info,
        "A call to begin_transaction() must provide memory allocation information.");

    retail_assert(s_fd_log == -1, "fd log should be uninitialized!");

    // Currently we don't need to alter any server-side state for opening a transaction.
    FlatBufferBuilder builder;

    // Allocate a new begin_ts for this txn and initialize its entry. Loop until
    // we are able to install the entry before being invalidated by another
    // thread.
    gaia_txn_id_t begin_ts = c_invalid_gaia_txn_id;
    while (begin_ts == c_invalid_gaia_txn_id)
    {
        begin_ts = txn_begin();
    }
    s_txn_id = begin_ts;

    // Ensure that there are no undecided txns in our snapshot window.
    validate_txns_in_range(s_last_applied_commit_ts_upper_bound + 1, s_txn_id);

    // REVIEW: we could make this a session thread-local to avoid dynamic
    // allocation per txn.
    std::vector<int> txn_log_fds;
    get_txn_log_fds_for_snapshot(begin_ts, txn_log_fds);

    // Send the reply message to the client, with the number of txn log fds to
    // be sent later.
    // REVIEW: We could optimize the fast path by piggybacking some small fixed
    // number of log fds on the initial reply message.
    build_server_reply(builder, session_event_t::BEGIN_TXN, old_state, new_state, s_txn_id, txn_log_fds.size());
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Send all txn log fds to the client in an additional sequence of dummy messages.
    // We need a 1-byte dummy message buffer due to our datagram size convention.
    uint8_t msg_buf[1] = {0};
    size_t fds_written_count = 0;
    while (fds_written_count < txn_log_fds.size())
    {
        size_t fds_to_write_count = std::min(c_max_fd_count, txn_log_fds.size() - fds_written_count);
        send_msg_with_fds(
            s_session_socket, txn_log_fds.data() + fds_written_count, fds_to_write_count, msg_buf, sizeof(msg_buf));
        fds_written_count += fds_to_write_count;
    }

    // Now we need to close all the duplicated log fds in the buffer.
    for (auto& dup_fd : txn_log_fds)
    {
        // Each log fd should still be valid.
        retail_assert(is_fd_valid(dup_fd), "invalid fd!");
        close_fd(dup_fd);
    }
}

void server::get_txn_log_fds_for_snapshot(gaia_txn_id_t begin_ts, std::vector<int>& txn_log_fds)
{
    // Take a snapshot of the watermark and scan backward from begin_ts,
    // stopping at either the saved watermark or the first commit_ts whose log
    // fd has been invalidated. This avoids having our scan race the
    // concurrently advancing watermark.

    gaia_txn_id_t last_applied_commit_ts = s_last_applied_commit_ts_upper_bound;
    for (gaia_txn_id_t ts = begin_ts - 1; ts > last_applied_commit_ts; --ts)
    {
        if (is_commit_ts(ts))
        {
            retail_assert(
                is_txn_decided(ts),
                "Undecided commit_ts found in snapshot window!");
            if (is_txn_committed(ts))
            {
                // Since the watermark could advance past its saved value, we
                // need to be sure that we don't send an invalidated and closed
                // log fd, so we validate and duplicate each log fd using the
                // safe_fd_from_ts class before sending it to the client. We set
                // auto_close_fd = false in the safe_fd_from_ts constructor
                // because we need to save the dup fds in the buffer until we
                // pass them to sendmsg().
                try
                {
                    safe_fd_from_ts committed_txn_log_fd(ts, false);
                    int local_log_fd = committed_txn_log_fd.get_fd();
                    txn_log_fds.push_back(local_log_fd);
                }
                catch (const invalid_log_fd&)
                {
                    // We ignore an invalidated fd since its log has already
                    // been applied to the shared locator view, so we don't need
                    // to send it to the client anyway. This means all preceding
                    // committed txns have already been applied to the shared
                    // locator view, so we can terminate the scan early.
                    break;
                }
            }
        }
    }

    // Since we scan the snapshot window backward and append fds to the buffer,
    // they are in reverse timestamp order.
    std::reverse(std::begin(txn_log_fds), std::end(txn_log_fds));
}

void server::handle_rollback_txn(
    int* fds, size_t fd_count, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::ROLLBACK_TXN, c_message_unexpected_event_received);

    // This message should only be received while a transaction is in progress.
    retail_assert(
        old_state == session_state_t::TXN_IN_PROGRESS && new_state == session_state_t::CONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    retail_assert(s_fd_log == -1, "fd log should be uninitialized!");

    // Get the log fd and free it if the client sends it.
    // The client will not send the txn log to the server if a read-only txn was rolled back.
    if (fds && fd_count == 1)
    {
        s_fd_log = *fds;
        retail_assert(s_fd_log != -1, c_message_uninitialized_fd_log);
    }

    // Release all txn resources and mark the txn's begin_ts entry as terminated.
    txn_rollback();
}

void server::handle_commit_txn(
    int* fds, size_t fd_count, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::COMMIT_TXN, c_message_unexpected_event_received);

    // This message should only be received while a transaction is in progress.
    retail_assert(
        old_state == session_state_t::TXN_IN_PROGRESS && new_state == session_state_t::TXN_COMMITTING,
        c_message_current_event_is_inconsistent_with_state_transition);

    // Get the log fd and mmap it.
    retail_assert(fds && fd_count == 1, "Invalid fd data!");
    s_fd_log = *fds;
    retail_assert(s_fd_log != -1, c_message_uninitialized_fd_log);

    // We need to keep the log fd around until it's applied to the shared
    // locator view, so we only close the log fd if an exception is thrown.
    // Aborted txns only have their log fds invalidated and closed after the
    // watermark advances, to preserve the invariant that invalidation can only
    // follow the watermark.
    auto cleanup_log_fd = make_scope_guard([&]() {
        close_fd(s_fd_log);
    });

    // Check that the log memfd was sealed for writes.
    int seals = ::fcntl(s_fd_log, F_GET_SEALS);
    if (seals == -1)
    {
        throw_system_error("fcntl(F_GET_SEALS) failed!");
    }
    retail_assert(seals == (F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE), "Unexpected seals on log fd!");

    // Linux won't let us create a shared read-only mapping if F_SEAL_WRITE is set,
    // which seems contrary to the manpage for fcntl(2).
    map_fd(s_log, get_fd_size(s_fd_log), PROT_READ, MAP_PRIVATE, s_fd_log, 0);
    // Unconditionally unmap the log segment on exit.
    auto cleanup_log = make_scope_guard([]() {
        unmap_fd(s_log, s_log->size());
    });

    // Actually commit the transaction.
    bool success = txn_commit();

    // Ideally, we would immediately clean up the log of an aborted txn, but
    // this complicates reasoning about safe concurrent validation (we want to
    // maintain the invariant that a log is never cleaned up until the watermark
    // advances past its txn's commit_ts). Once txn_commit() returns, only a
    // thread advancing the watermark can clean up this txn's log fd.
    cleanup_log_fd.dismiss();
    session_event_t decision = success ? session_event_t::DECIDE_TXN_COMMIT : session_event_t::DECIDE_TXN_ABORT;

    // Server-initiated state transition! (Any issues with reentrant handlers?)
    apply_transition(decision, nullptr, nullptr, 0);
}

void server::handle_request_memory(
    int*, size_t, session_event_t event, const void* event_data, session_state_t old_state, session_state_t new_state)
{
    retail_assert(
        event == session_event_t::REQUEST_MEMORY,
        "Incorrect event type for requesting more memory.");

    // This event never changes session state.
    retail_assert(
        old_state == new_state,
        "Requesting more memory shouldn't cause session state to change.");

    // This API is only invoked mid transaction.
    retail_assert(
        old_state == session_state_t::TXN_IN_PROGRESS,
        "Old state when requesting more memory from server should be TXN_IN_PROGRESS.");

    auto request = static_cast<const client_request_t*>(event_data);
    retail_assert(
        request->data_type() == request_data_t::memory_info,
        c_message_unexpected_request_data_type);

    auto memory_request_size_bytes = static_cast<size_t>(request->data_as_memory_info()->memory_request_size());
    auto object_address_offset = allocate_from_memory_manager(memory_request_size_bytes);

    FlatBufferBuilder builder;
    build_server_reply(builder, session_event_t::REQUEST_MEMORY, old_state, new_state, s_txn_id, 0, object_address_offset);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());
}

void server::handle_decide_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(
        event == session_event_t::DECIDE_TXN_COMMIT || event == session_event_t::DECIDE_TXN_ABORT,
        c_message_unexpected_event_received);

    retail_assert(
        old_state == session_state_t::TXN_COMMITTING && new_state == session_state_t::CONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    // We need to clear transactional state after the decision has been
    // returned, but don't need to free any resources.
    auto cleanup = make_scope_guard([&]() {
        s_txn_id = c_invalid_gaia_txn_id;
        s_fd_log = -1;
    });

    FlatBufferBuilder builder;
    build_server_reply(builder, event, old_state, new_state, s_txn_id);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Update watermarks and perform associated maintenance tasks. This will
    // block new transactions on this session thread, but that is a feature, not
    // a bug, since it provides natural backpressure on clients who submit
    // long-running transactions that prevent old versions and logs from being
    // freed. This approach helps keep the system from accumulating more
    // deferred work than it can ever retire, which is a problem with performing
    // all maintenance asynchronously in the background. Allowing this work to
    // delay beginning new transactions but not delay committing the current
    // transaction seems like a good compromise.
    perform_maintenance();
}

void server::handle_client_shutdown(
    int*, size_t, session_event_t event, const void*, session_state_t, session_state_t new_state)
{
    retail_assert(
        event == session_event_t::CLIENT_SHUTDOWN,
        c_message_unexpected_event_received);

    retail_assert(
        new_state == session_state_t::DISCONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    // If this event is received, the client must have closed the write end of the socket
    // (equivalent of sending a FIN), so we need to do the same. Closing the socket
    // will send a FIN to the client, so they will read EOF and can close the socket
    // as well. We can just set the shutdown flag here, which will break out of the poll
    // loop and immediately close the socket. (If we received EOF from the client after
    // we closed our write end, then we would be calling shutdown(SHUT_WR) twice, which
    // is another reason to just close the socket.)
    s_session_shutdown = true;

    // If the session had an active txn, clean up all its resources.
    if (s_txn_id != c_invalid_gaia_txn_id)
    {
        txn_rollback();
    }
}

void server::handle_server_shutdown(
    int*, size_t, session_event_t event, const void*, session_state_t, session_state_t new_state)
{
    retail_assert(
        event == session_event_t::SERVER_SHUTDOWN,
        c_message_unexpected_event_received);

    retail_assert(
        new_state == session_state_t::DISCONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    // This transition should only be triggered on notification of the server shutdown event.
    // Since we are about to shut down, we can't wait for acknowledgment from the client and
    // should just close the session socket. As noted above, setting the shutdown flag will
    // immediately break out of the poll loop and close the session socket.
    s_session_shutdown = true;
}

std::pair<int, int> server::get_stream_socket_pair()
{
    // Create a connected pair of datagram sockets, one of which we will keep
    // and the other we will send to the client.
    // We use SOCK_SEQPACKET because it supports both datagram and connection
    // semantics: datagrams allow buffering without framing, and a connection
    // ensures that client returns EOF after server has called shutdown(SHUT_WR).
    int socket_pair[2] = {-1};
    if (-1 == ::socketpair(PF_UNIX, SOCK_SEQPACKET, 0, socket_pair))
    {
        throw_system_error("socketpair() failed!");
    }

    auto [client_socket, server_socket] = socket_pair;
    // We need to use the initializer + mutable hack to capture structured bindings in a lambda.
    auto socket_cleanup = make_scope_guard([client_socket = client_socket,
                                            server_socket = server_socket]() mutable {
        close_fd(client_socket);
        close_fd(server_socket);
    });

    // Set server socket to be nonblocking, since we use it within an epoll loop.
    set_non_blocking(server_socket);

    socket_cleanup.dismiss();
    return std::pair{client_socket, server_socket};
}

void server::handle_request_stream(
    int*, size_t, session_event_t event, const void* event_data, session_state_t old_state, session_state_t new_state)
{
    retail_assert(
        event == session_event_t::REQUEST_STREAM,
        c_message_unexpected_event_received);

    // This event never changes session state.
    retail_assert(
        old_state == new_state,
        c_message_current_event_is_inconsistent_with_state_transition);

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
        c_message_unexpected_request_data_type);

    auto type = static_cast<gaia_type_t>(request->data_as_table_scan()->type_id());
    auto id_generator = get_id_generator_for_type(type);

    // We can't use structured binding names in a lambda capture list.
    int client_socket, server_socket;
    std::tie(client_socket, server_socket) = get_stream_socket_pair();

    // The client socket should unconditionally be closed on exit since it's
    // duplicated when passed to the client and we no longer need it on the
    // server.
    auto client_socket_cleanup = make_scope_guard([&]() {
        close_fd(client_socket);
    });
    auto server_socket_cleanup = make_scope_guard([&]() {
        close_fd(server_socket);
    });

    start_stream_producer(server_socket, id_generator);

    // Transfer ownership of the server socket to the stream producer thread.
    server_socket_cleanup.dismiss();

    // Any exceptions after this point will close the server socket, ensuring the producer thread terminates.
    // However, its destructor will not run until the session thread exits and joins the producer thread.
    FlatBufferBuilder builder;
    build_server_reply(builder, event, old_state, new_state);
    send_msg_with_fds(s_session_socket, &client_socket, 1, builder.GetBufferPointer(), builder.GetSize());
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
        "No allowed state transition from state '"
        + std::string(EnumNamesession_state_t(s_session_state))
        + "' with event '"
        + std::string(EnumNamesession_event_t(event))
        + "'.");
}

void server::build_server_reply(
    FlatBufferBuilder& builder,
    session_event_t event,
    session_state_t old_state,
    session_state_t new_state,
    gaia_txn_id_t txn_id,
    size_t log_fd_count,
    address_offset_t object_address_offset)
{
    flatbuffers::Offset<server_reply_t> server_reply;
    if (object_address_offset)
    {
        const auto memory_allocation_reply = Creatememory_allocation_info_t(builder, object_address_offset);
        server_reply = Createserver_reply_t(
            builder, event, old_state, new_state, reply_data_t::memory_allocation_info, memory_allocation_reply.Union());
    }
    else
    {
        const auto transaction_info = Createtransaction_info_t(builder, txn_id, log_fd_count);
        server_reply = Createserver_reply_t(
            builder, event, old_state, new_state, reply_data_t::transaction_info, transaction_info.Union());
    }
    const auto message = Createmessage_t(builder, any_message_t::reply, server_reply.Union());
    builder.Finish(message);
}

void server::clear_shared_memory()
{
    unmap_fd(s_shared_locators, sizeof(*s_shared_locators));
    close_fd(s_fd_locators);

    unmap_fd(s_counters, sizeof(*s_counters));
    close_fd(s_fd_counters);

    unmap_fd(s_data, sizeof(*s_data));
    close_fd(s_fd_data);

    unmap_fd(s_id_index, sizeof(*s_id_index));
    close_fd(s_fd_id_index);
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

    retail_assert(s_fd_locators == -1, "Locator fd should not be valid!");
    retail_assert(s_fd_counters == -1, "Counters fd should not be valid!");
    retail_assert(s_fd_data == -1, "Data fd should not be valid!");
    retail_assert(s_fd_id_index == -1, "ID index fd should not be valid!");

    retail_assert(!s_shared_locators, "Locators memory should be reset!");
    retail_assert(!s_counters, "Counters memory should be reset!");
    retail_assert(!s_data, "Data memory should be reset!");
    retail_assert(!s_id_index, "ID index memory should be reset!");

    s_fd_locators = ::memfd_create(c_sch_mem_locators, MFD_ALLOW_SEALING);
    if (s_fd_locators == -1)
    {
        throw_system_error(c_message_memfd_create_failed);
    }

    s_fd_counters = ::memfd_create(c_sch_mem_counters, MFD_ALLOW_SEALING);
    if (s_fd_counters == -1)
    {
        throw_system_error(c_message_memfd_create_failed);
    }

    s_fd_data = ::memfd_create(c_sch_mem_data, MFD_ALLOW_SEALING);
    if (s_fd_data == -1)
    {
        throw_system_error(c_message_memfd_create_failed);
    }

    s_fd_id_index = ::memfd_create(c_sch_mem_id_index, MFD_ALLOW_SEALING);
    if (s_fd_id_index == -1)
    {
        throw_system_error(c_message_memfd_create_failed);
    }

    truncate_fd(s_fd_locators, sizeof(*s_shared_locators));
    truncate_fd(s_fd_counters, sizeof(*s_counters));
    truncate_fd(s_fd_data, sizeof(*s_data));
    truncate_fd(s_fd_id_index, sizeof(*s_id_index));

    // s_shared_locators uses sizeof(gaia_offset_t) * c_max_locators = 32GB of virtual address space.
    //
    // s_data uses (64B) * c_max_locators = 256GB of virtual address space.
    //
    // s_id_index uses (32B) * c_max_locators = 128GB of virtual address space
    // (assuming 4-byte alignment). We could eventually shrink this to
    // 4B/locator (assuming 4-byte locators), or 16GB, if we can assume that
    // gaia_ids are sequentially allocated and seldom deleted, so we can just
    // use an array of locators indexed by gaia_id.
    //
    // Note that unless we supply the MAP_POPULATE flag to mmap(), only the
    // pages we actually use will ever be allocated. However, Linux may refuse
    // to allocate the virtual memory we requested if overcommit is disabled
    // (i.e., /proc/sys/vm/overcommit_memory = 2). Using MAP_NORESERVE (don't
    // reserve swap space) should ensure that overcommit always succeeds, unless
    // it is disabled. We may need to document the requirement that overcommit
    // is not disabled (i.e., /proc/sys/vm/overcommit_memory != 2).
    //
    // Alternatively, we could use the more tedious but reliable approach of
    // using mmap(PROT_NONE) and calling mprotect(PROT_READ|PROT_WRITE) on any
    // pages we need to use (this is analogous to VirtualAlloc(MEM_RESERVE)
    // followed by VirtualAlloc(MEM_COMMIT) in Windows).
    map_fd(s_shared_locators, sizeof(*s_shared_locators), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, s_fd_locators, 0);
    map_fd(s_counters, sizeof(*s_counters), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, s_fd_counters, 0);
    map_fd(s_data, sizeof(*s_data), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, s_fd_data, 0);
    map_fd(s_id_index, sizeof(*s_id_index), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, s_fd_id_index, 0);

    init_memory_manager();

    // Populate shared memory from the persistent log and snapshot.
    recover_db();

    cleanup_memory.dismiss();
}

void server::init_memory_manager()
{
    s_memory_manager.reset();
    s_memory_manager = make_unique<memory_manager_t>();
    s_memory_manager->manage(reinterpret_cast<uint8_t*>(s_data->objects), sizeof(s_data->objects));

    auto deallocate_object_fn = [=](gaia_offset_t offset) {
        s_memory_manager->free_old_offset(get_address_offset(offset));
    };
    register_object_deallocator(deallocate_object_fn);
}

address_offset_t server::allocate_object(
    gaia_locator_t locator,
    size_t size)
{
    address_offset_t offset = s_memory_manager->allocate(size + sizeof(db_object_t));
    if (offset == c_invalid_offset)
    {
        throw memory_allocation_error("Memory manager ran out of memory during call to allocate().");
    }
    update_locator(locator, offset);
    return offset;
}

// Use a huge sparse array to store the txn info, where each entry is indexed by
// its begin or commit timestamp. We use 2^45 = 32TB of virtual address space
// for this array, but only the pages we actually use will ever be allocated.
// From https://www.kernel.org/doc/Documentation/vm/overcommit-accounting, it
// appears that MAP_NORESERVE is ignored in overcommit mode 2 (never
// overcommit), so we may need to check the `vm.overcommit_memory` sysctl and
// fail if it's set to 2. Alternatively, we could use the more tedious but
// reliable approach of using mmap(PROT_NONE) and calling
// mprotect(PROT_READ|PROT_WRITE) on any pages we actually need to use (this is
// analogous to VirtualAlloc(MEM_RESERVE) followed by VirtualAlloc(MEM_COMMIT)
// in Windows). 2^45 bytes of virtual address space holds 2^42 8-byte words, so
// we can allocate at most 2^42 timestamps over the lifetime of the process. If
// we allocate timestamps at a rate of 1M/s, we can run for about 2^22 seconds,
// or about a month and a half. If we allocate only 1K/s, we can run a thousand
// times longer: 2^32 seconds = ~128 years. If running out of timestamps is a
// concern, we can just convert the array to a circular buffer and store a
// wraparound counter separately.

// Each entry in the txn_info array is an 8-byte word, with the high bit
// indicating whether the index represents a begin or commit timestamp (they are
// allocated from the same counter). The next two bits indicate the current
// state of the txn state machine. The txn state machine's states and
// transitions are as follows: TXN_NONE -> TXN_ACTIVE -> (TXN_SUBMITTED ->
// (TXN_COMMITTED, TXN_ABORTED), TXN_TERMINATED). TXN_NONE is necessarily
// encoded as 0 since all entries of the array are zeroed when a page is
// allocated. We also introduce the pseudo-state TXN_DECIDED, encoded as the
// shared set bit in TXN_COMMITTED and TXN_ABORTED. Each active txn may assume
// the states TXN_NONE, TXN_ACTIVE, TXN_SUBMITTED, TXN_TERMINATED, while
// each submitted txn may assume the states TXN_SUBMITTED, TXN_COMMITTED,
// TXN_ABORTED, so we need 2 bits for each txn entry to represent all possible
// states. Combined with the begin/commit bit, we need to reserve the 3 high
// bits of each 64-bit entry for txn state, leaving 61 bits for other purposes.
// The remaining bits are used as follows. In the TXN_NONE state, all bits are
// 0. In the TXN_ACTIVE state, the non-state bits are currently unused, but
// they could be used for active txn information like session thread ID, session
// socket, session userfaultfd, etc. In the TXN_SUBMITTED state, the remaining
// bits always hold a commit timestamp (which is guaranteed to be at most 42
// bits per above). The commit timestamp serves as a forwarding pointer to the
// submitted txn entry for this txn. Note that the commit timestamp entry for a
// txn may be set before its begin timestamp entry has been updated, since
// multiple updates to the array cannot be made atomically without locks. (This
// is similar to insertion into a lock-free singly-linked list, where the new
// node is linked to its successor before its predecessor is linked to the new
// node.) Our algorithms are tolerant to this transient inconsistency. A commit
// timestamp entry always holds the submitted txn's log fd in the low 16 bits
// following the 3 high bits used for state (we assume that all log fds fit into
// 16 bits), and the txn's begin timestamp in the low 42 bits.

// We can always find the watermark by just scanning the txn_info array until we
// find the first begin timestamp entry not in state TXN_TERMINATED. However, we
// need to know where to start scanning from since pages toward the beginning of
// the array may have already been deallocated (as part of advancing the
// watermark, once all txn logs preceding the watermark have been scanned for
// garbage and deallocated, all pages preceding the new watermark's position in
// the array can be freed using madvise(MADV_FREE)). We can store a global
// watermark variable to cache this information, which could always be stale but
// tells us where to start scanning to find the current watermark, since the
// watermark only moves forward. This global variable is also set as part of
// advancing the watermark on termination of the oldest active txn, which is
// delegated to that txn's session thread.

void server::init_txn_info()
{
    // We reserve 2^45 bytes = 32TB of virtual address space. YOLO.
    constexpr size_t c_size_in_bytes = (1ULL << c_txn_ts_bits) * sizeof(*s_txn_info);

    // Create an anonymous private mapping with MAP_NORESERVE to indicate that
    // we don't care about reserving swap space.
    // REVIEW: If this causes problems on systems that disable overcommit, we
    // can just use PROT_NONE and mprotect(PROT_READ|PROT_WRITE) each page as we
    // need it.
    if (s_txn_info)
    {
        unmap_fd(s_txn_info, c_size_in_bytes);
    }
    map_fd(
        s_txn_info,
        c_size_in_bytes,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
        -1,
        0);
}

void server::recover_db()
{
    // If persistence is disabled, then this is a no-op.
    if (!(s_persistence_mode == persistence_mode_t::e_disabled))
    {
        // We could get here after a server reset with --disable-persistence-after-recovery,
        // in which case we need to recover from the original persistent image.
        if (!rdb)
        {
            rdb = make_unique<gaia::db::persistent_store_manager>();
            if (s_persistence_mode == persistence_mode_t::e_reinitialized_on_startup)
            {
                rdb->destroy_persistent_store();
            }
            rdb->open();
            rdb->recover();
        }
    }

    // If persistence is disabled after recovery, then destroy the RocksDB
    // instance.
    if (s_persistence_mode == persistence_mode_t::e_disabled_after_recovery)
    {
        rdb.reset();
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

    cerr << "Caught signal '" << ::strsignal(signum) << "'." << endl;

    signal_eventfd(s_server_shutdown_eventfd);
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
        throw_system_error("Socket creation failed!");
    }
    auto socket_cleanup = make_scope_guard([&]() { close_fd(listening_socket); });

    // Initialize the socket address structure.
    sockaddr_un server_addr = {0};
    server_addr.sun_family = AF_UNIX;

    // The socket name (minus its null terminator) needs to fit into the space
    // in the server address structure after the prefix null byte.
    static_assert(sizeof(c_db_server_socket_name) <= sizeof(server_addr.sun_path) - 1);

    // We prepend a null byte to the socket name so the address is in the
    // (Linux-exclusive) "abstract namespace", i.e., not bound to the
    // filesystem.
    ::strncpy(&server_addr.sun_path[1], c_db_server_socket_name, sizeof(server_addr.sun_path) - 1);

    // Bind the socket to the address and start listening for connections.
    // The socket name is not null-terminated in the address structure, but
    // we need to add an extra byte for the null byte prefix.
    socklen_t server_addr_size = sizeof(server_addr.sun_family) + 1 + ::strlen(&server_addr.sun_path[1]);
    if (-1 == ::bind(listening_socket, reinterpret_cast<struct sockaddr*>(&server_addr), server_addr_size))
    {
        throw_system_error("bind() failed!");
    }
    if (-1 == ::listen(listening_socket, 0))
    {
        throw_system_error("listen() failed!");
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
        throw_system_error("getsockopt(SO_PEERCRED) failed!");
    }

    // Disable client authentication until we can figure out
    // how to fix the Postgres tests.
    // Client must have same effective user ID as server.
    // return (cred.uid == ::geteuid());

    return true;
}

// We adopt a lazy GC approach to freeing thread resources, rather than having
// each thread clean up after itself on exit. This approach allows us to avoid
// any synchronization between the exiting thread and its owning thread, as well
// as the awkwardness of passing each thread a reference to its owning object.
// In general, any code which creates a new thread is expected to call this
// function to compensate for the "garbage" it is adding to the system.
//
// Removing a thread entry is O(1) (because we swap it with the last element and
// truncate the last element), so the whole scan with removals is O(n).
static void reap_exited_threads(std::vector<std::thread>& threads)
{
    for (auto iter = threads.begin(); iter != threads.end();)
    {
        // Test if the thread has already exited (this is possible with the
        // pthreads API but not with the std::thread API).
        auto handle = iter->native_handle();

        // pthread_kill(0) returns 0 if the thread is still running, and ESRCH
        // otherwise (unless it has already been detached or joined, in which
        // case the thread ID may be invalid or reused, possibly causing a
        // segfault). We never use a thread ID after the thread has been joined
        // (and we never detach threads), so we should be OK.
        //
        // https://man7.org/linux/man-pages/man3/pthread_kill.3.html
        // "POSIX.1-2008 recommends that if an implementation detects the use of
        // a thread ID after the end of its lifetime, pthread_kill() should
        // return the error ESRCH. The glibc implementation returns this error
        // in the cases where an invalid thread ID can be detected. But note
        // also that POSIX says that an attempt to use a thread ID whose
        // lifetime has ended produces undefined behavior, and an attempt to use
        // an invalid thread ID in a call to pthread_kill() can, for example,
        // cause a segmentation fault."
        //
        // https://man7.org/linux/man-pages/man3/pthread_self.3.html
        // "A thread ID may be reused after a terminated thread has been joined,
        // or a detached thread has terminated."
        int error = ::pthread_kill(handle, 0);

        if (error == 0)
        {
            // The thread is still running, so do nothing.
            ++iter;
        }
        else if (error == ESRCH)
        {
            // If this thread has already exited, then join it and deallocate
            // its object to release both memory and thread-related system
            // resources.
            retail_assert(iter->joinable(), c_message_thread_must_be_joinable);
            iter->join();

            // Move the last element into the current entry.
            *iter = std::move(threads.back());
            threads.pop_back();
        }
        else
        {
            // Throw on all other errors (e.g., if the thread has been detached
            // or joined).
            throw_system_error("pthread_kill(0) failed!", error);
        }
    }
}

void server::client_dispatch_handler()
{
    // Register session cleanup handler first, so we can execute it last.
    auto session_cleanup = make_scope_guard([]() {
        for (auto& thread : s_session_threads)
        {
            retail_assert(thread.joinable(), c_message_thread_must_be_joinable);
            thread.join();
        }
        // All session threads have been joined, so they can be destroyed.
        s_session_threads.clear();
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
        throw_system_error(c_message_epoll_create1_failed);
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
            throw_system_error(c_message_epoll_ctl_failed);
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
            throw_system_error(c_message_epoll_wait_failed);
        }

        for (int i = 0; i < ready_fd_count; ++i)
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
                    throw_system_error("Client socket error!", error);
                }
                else if (ev.data.fd == s_server_shutdown_eventfd)
                {
                    throw_system_error("Shutdown eventfd error!");
                }
            }

            // At this point, we should only get EPOLLIN.
            retail_assert(ev.events == EPOLLIN, c_message_unexpected_event_type);

            if (ev.data.fd == s_listening_socket)
            {
                int session_socket = ::accept(s_listening_socket, nullptr, nullptr);
                if (session_socket == -1)
                {
                    throw_system_error("accept() failed!");
                }
                if (authenticate_client_socket(session_socket))
                {
                    // First reap any session threads that have terminated (to
                    // avoid memory and system resource leaks).
                    reap_exited_threads(s_session_threads);
                    // Create session thread.
                    s_session_threads.emplace_back(session_handler, session_socket);
                }
                else
                {
                    close_fd(session_socket);
                }
            }
            else if (ev.data.fd == s_server_shutdown_eventfd)
            {
                consume_eventfd(s_server_shutdown_eventfd);
                return;
            }
            else
            {
                // We don't monitor any other fds.
                retail_assert(false, c_message_unexpected_fd);
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
        throw_system_error(c_message_epoll_create1_failed);
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
            throw_system_error(c_message_epoll_ctl_failed);
        }
    }
    epoll_event events[std::size(fds)];

    // Event to signal session-owned threads to terminate.
    s_session_shutdown_eventfd = make_eventfd();
    auto owned_threads_cleanup = make_scope_guard([]() {
        // Signal all session-owned threads to terminate.
        signal_eventfd(s_session_shutdown_eventfd);

        // Wait for all session-owned threads to terminate.
        for (auto& thread : s_session_owned_threads)
        {
            retail_assert(thread.joinable(), c_message_thread_must_be_joinable);
            thread.join();
        }

        // All session-owned threads have been joined, so they can be destroyed.
        s_session_owned_threads.clear();

        // All session-owned threads have received the session shutdown
        // notification, so we can close the eventfd.
        close_fd(s_session_shutdown_eventfd);
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
            throw_system_error(c_message_epoll_wait_failed);
        }

        session_event_t event = session_event_t::NOP;
        const void* event_data = nullptr;

        // Buffer used to send and receive all message data.
        uint8_t msg_buf[c_max_msg_size] = {0};

        // Buffer used to receive file descriptors.
        int fd_buf[c_max_fd_count] = {-1};
        size_t fd_buf_size = std::size(fd_buf);
        int* fds = nullptr;
        size_t fd_count = 0;

        // If the shutdown flag is set, we need to exit immediately before
        // processing the next ready fd.
        for (int i = 0; i < ready_fd_count && !s_session_shutdown; ++i)
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
                    retail_assert(!(ev.events & EPOLLERR), c_message_epollerr_flag_should_not_be_set);
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

                    event_data = static_cast<const void*>(request);

                    if (fd_buf_size > 0)
                    {
                        fds = fd_buf;
                        fd_count = fd_buf_size;
                    }
                }
                else
                {
                    // We don't register for any other events.
                    retail_assert(false, c_message_unexpected_event_type);
                }
            }
            else if (ev.data.fd == s_server_shutdown_eventfd)
            {
                retail_assert(ev.events == EPOLLIN, "Expected EPOLLIN event type!");
                consume_eventfd(s_server_shutdown_eventfd);
                event = session_event_t::SERVER_SHUTDOWN;
            }
            else
            {
                // We don't monitor any other fds.
                retail_assert(false, c_message_unexpected_fd);
            }

            retail_assert(event != session_event_t::NOP, c_message_unexpected_event_type);

            // The transition handlers are the only places we currently call
            // send_msg_with_fds(). We need to handle a peer_disconnected
            // exception thrown from that method (translated from EPIPE).
            try
            {
                apply_transition(event, event_data, fds, fd_count);
            }
            catch (const peer_disconnected& e)
            {
                cerr << "client socket error: " << e.what() << endl;
                s_session_shutdown = true;
            }
        }
    }
}

template <typename T_element_type>
void server::stream_producer_handler(
    int stream_socket, int cancel_eventfd, std::function<std::optional<T_element_type>()> generator_fn)
{
    // We only support fixed-width integer types for now to avoid framing.
    static_assert(std::is_integral<T_element_type>::value, "Generator function must return an integer.");

    // The session thread gave the producer thread ownership of this socket.
    auto socket_cleanup = make_scope_guard([&]() {
        // We can rely on close_fd() to perform the equivalent of shutdown(SHUT_RDWR),
        // since we hold the only fd pointing to this socket.
        close_fd(stream_socket);
    });

    // Verify that the socket is the correct type for the semantics we assume.
    check_socket_type(stream_socket, SOCK_SEQPACKET);

    // Check that our stream socket is non-blocking (so we don't accidentally block in write()).
    retail_assert(is_non_blocking(stream_socket), "Stream socket is in blocking mode!");

    auto gen_iter = generator_iterator_t<T_element_type>(generator_fn);

    int epoll_fd = ::epoll_create1(0);
    if (epoll_fd == -1)
    {
        throw_system_error(c_message_epoll_create1_failed);
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
        throw_system_error(c_message_epoll_ctl_failed);
    }

    epoll_event cancel_ev = {0};
    cancel_ev.events = EPOLLIN;
    cancel_ev.data.fd = cancel_eventfd;
    if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cancel_eventfd, &cancel_ev))
    {
        throw_system_error(c_message_epoll_ctl_failed);
    }

    epoll_event events[2];
    bool producer_shutdown = false;

    // The userspace buffer that we use to construct a batch datagram message.
    std::vector<T_element_type> batch_buffer;

    // We need to call reserve() rather than the "sized" constructor to avoid changing size().
    batch_buffer.reserve(c_stream_batch_size);

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
            throw_system_error(c_message_epoll_wait_failed);
        }
        // If the shutdown flag is set, we need to exit immediately before
        // processing the next ready fd.
        for (int i = 0; i < ready_fd_count && !producer_shutdown; ++i)
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
                    cerr << "Stream socket error: '" << ::strerror(error) << "'." << endl;
                    producer_shutdown = true;
                }
                else if (ev.events & EPOLLHUP)
                {
                    // This flag is unmaskable, so we don't need to register for it.
                    // We shold get this when the client has closed its end of the socket.
                    retail_assert(!(ev.events & EPOLLERR), c_message_epollerr_flag_should_not_be_set);
                    producer_shutdown = true;
                }
                else if (ev.events & EPOLLOUT)
                {
                    retail_assert(
                        !(ev.events & (EPOLLERR | EPOLLHUP)),
                        "EPOLLERR and EPOLLHUP flags should not be set!");

                    // Write to the send buffer until we exhaust either the iterator or the buffer free space.
                    while (gen_iter && (batch_buffer.size() < c_stream_batch_size))
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
                            retail_assert(errno != EAGAIN && errno != EWOULDBLOCK, c_message_unexpected_errno_value);
                            // Log the error and break out of the poll loop.
                            cerr << "Stream socket error: '" << ::strerror(errno) << "'." << endl;
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
                    retail_assert(false, c_message_unexpected_event_type);
                }
            }
            else if (ev.data.fd == cancel_eventfd)
            {
                retail_assert(ev.events == EPOLLIN, c_message_unexpected_event_type);
                consume_eventfd(cancel_eventfd);
                producer_shutdown = true;
            }
            else
            {
                // We don't monitor any other fds.
                retail_assert(false, c_message_unexpected_fd);
            }
        }
    }
}

template <typename T_element_type>
void server::start_stream_producer(int stream_socket, std::function<std::optional<T_element_type>()> generator_fn)
{
    // First reap any owned threads that have terminated (to avoid memory and
    // system resource leaks).
    reap_exited_threads(s_session_owned_threads);

    // Create stream producer thread.
    s_session_owned_threads.emplace_back(
        stream_producer_handler<T_element_type>, stream_socket, s_session_shutdown_eventfd, generator_fn);
}

std::function<std::optional<gaia_id_t>()> server::get_id_generator_for_type(gaia_type_t type)
{
    gaia_locator_t locator = 0;

    // Fix end of locator segment for length of scan, since it can change during scan.
    gaia_locator_t last_locator = s_counters->last_locator;

    return [=]() mutable -> std::optional<gaia_id_t> {
        // REVIEW: Now that the locator segment is no longer locked, we have no
        // transactional guarantees when reading from it. We need to either have
        // server-side transactions (which would use the same begin timestamp as
        // the client), or non-transactional structures mapping types to all IDs
        // or locators ever allocated for that type, so we didn't have to read
        // an object version to determine the type of an ID or locator. Given
        // such a non-transactional index, the client could post-filter results
        // from the server for locators which were unset or deleted in its
        // transactional view. For now, this API is just a prototype anyway and
        // not strictly necessary (the client can do the same naive scan of all
        // objects in its view for their type), so we'll leave it unfixed
        // pending the introduction of efficient non-transactional "containers"
        // or type indexes.
        while (++locator && locator <= last_locator)
        {
            auto obj = locator_to_ptr(locator);
            if (obj && obj->type == type)
            {
                return obj->id;
            }
        }

        // Signal end of iteration.
        return std::nullopt;
    };
}

void server::validate_txns_in_range(gaia_txn_id_t start_ts, gaia_txn_id_t end_ts)
{
    // Scan txn table entries from start_ts to end_ts.
    // Invalidate any unknown entries and validate any undecided txns.
    for (gaia_txn_id_t ts = start_ts; ts < end_ts; ++ts)
    {
        // Fence off any txns that have allocated a commit_ts between start_ts
        // and end_ts but have not yet registered a commit_ts entry in the txn
        // table.
        invalidate_unknown_ts(ts);

        // Validate any undecided submitted txns.
        if (is_commit_ts(ts) && is_txn_validating(ts))
        {
            bool committed = validate_txn(ts);

            // Update the current txn's decided status.
            update_txn_decision(ts, committed);
        }
    }
}

const char* server::status_to_str(ts_entry_t ts_entry)
{
    retail_assert(
        (ts_entry != c_txn_entry_unknown) && (ts_entry != c_txn_entry_invalid),
        "Not a valid timestamp entry!");

    uint64_t status = get_status_from_entry(ts_entry);
    switch (status)
    {
    case c_txn_status_active:
        return "ACTIVE";
    case c_txn_status_submitted:
        return "SUBMITTED";
    case c_txn_status_terminated:
        return "TERMINATED";
    case c_txn_status_validating:
        return "VALIDATING";
    case c_txn_status_committed:
        return "COMMITTED";
    case c_txn_status_aborted:
        return "ABORTED";
    default:
        retail_assert(false, "Unexpected status flags!");
    }
    return nullptr;
}

void server::dump_ts_entry(gaia_txn_id_t ts)
{
    // NB: We generally cannot use the is_*_ts() functions since the entry could
    // change while we're reading it!
    ts_entry_t entry = s_txn_info[ts];
    std::bitset<c_txn_entry_bits> entry_bits(entry);

    cerr << "Timestamp entry for ts " << ts << ": " << entry_bits << endl;

    if (entry == c_txn_entry_unknown)
    {
        cerr << "UNKNOWN" << endl;
        return;
    }

    if (entry == c_txn_entry_invalid)
    {
        cerr << "INVALID" << endl;
        return;
    }

    cerr << "Type: " << (is_txn_entry_commit_ts(entry) ? "COMMIT" : "ACTIVE") << endl;
    cerr << "Status: " << status_to_str(entry) << endl;

    if (is_txn_entry_commit_ts(entry))
    {
        gaia_txn_id_t begin_ts = get_begin_ts(ts);
        // We can't recurse here since we'd just bounce back and forth between a
        // txn's begin_ts and commit_ts.
        ts_entry_t entry = s_txn_info[begin_ts];
        std::bitset<c_txn_entry_bits> entry_bits(entry);
        cerr << "Timestamp entry for commit_ts entry's begin_ts " << begin_ts << ": " << entry_bits << endl;
        cerr << "Log fd for commit_ts entry: " << get_txn_log_fd(ts) << endl;
    }

    if (is_txn_entry_begin_ts(entry) && is_txn_entry_submitted(entry))
    {
        gaia_txn_id_t commit_ts = get_commit_ts(ts);
        if (commit_ts != c_invalid_gaia_txn_id)
        {
            // We can't recurse here since we'd just bounce back and forth between a
            // txn's begin_ts and commit_ts.
            ts_entry_t entry = s_txn_info[commit_ts];
            std::bitset<c_txn_entry_bits> entry_bits(entry);
            cerr << "Timestamp entry for begin_ts entry's commit_ts " << commit_ts << ": " << entry_bits << endl;
        }
    }
}

// This is designed for implementing "fences" that can guarantee no thread can
// ever claim a timestamp entry, by marking that entry permanently invalid.
// Invalidation can only be performed on an "unknown" entry, not on any valid
// entry. When a session thread beginning or committing a txn finds that its
// begin_ts or commit_ts has been invalidated upon initializing the entry for
// that timestamp, it simply allocates another timestamp and retries. This is
// possible because we never publish a newly allocated timestamp until we know
// that its entry has been successfully initialized.
inline bool server::invalidate_unknown_ts(gaia_txn_id_t ts)
{
    // If the entry is not unknown, we can't invalidate it.
    if (!is_unknown_ts(ts))
    {
        return false;
    }

    ts_entry_t expected_entry = c_txn_entry_unknown;
    ts_entry_t invalid_entry = c_txn_entry_invalid;

    bool has_invalidated_entry = s_txn_info[ts].compare_exchange_strong(expected_entry, invalid_entry);
    // We don't consider TXN_SUBMITTED or TXN_TERMINATED to be valid prior states, since only the
    // submitting thread can transition the txn to these states.
    if (!has_invalidated_entry)
    {
        // NB: expected_entry is an inout argument holding the previous value on failure!
        retail_assert(
            expected_entry != c_txn_entry_unknown,
            "An unknown timestamp entry cannot fail to be invalidated!");
    }

    return has_invalidated_entry;
}

inline bool server::is_unknown_ts(gaia_txn_id_t ts)
{
    return s_txn_info[ts] == c_txn_entry_unknown;
}

inline bool server::is_invalid_ts(gaia_txn_id_t ts)
{
    return s_txn_info[ts] == c_txn_entry_invalid;
}

inline bool server::is_txn_entry_begin_ts(ts_entry_t ts_entry)
{
    // The "unknown" value has the commit bit unset, so we need to check it as well.
    return ((ts_entry != c_txn_entry_unknown) && ((ts_entry & c_txn_status_commit_mask) == 0));
}

inline bool server::is_begin_ts(gaia_txn_id_t ts)
{
    ts_entry_t ts_entry = s_txn_info[ts];
    return is_txn_entry_begin_ts(ts_entry);
}

inline bool server::is_txn_entry_commit_ts(ts_entry_t ts_entry)
{
    // The "invalid" value has the commit bit set, so we need to check it as well.
    return ((ts_entry != c_txn_entry_invalid) && ((ts_entry & c_txn_status_commit_mask) == c_txn_status_commit_mask));
}

inline bool server::is_commit_ts(gaia_txn_id_t ts)
{
    ts_entry_t ts_entry = s_txn_info[ts];
    return is_txn_entry_commit_ts(ts_entry);
}

inline bool server::is_txn_entry_submitted(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_submitted);
}

inline bool server::is_txn_submitted(gaia_txn_id_t begin_ts)
{
    retail_assert(is_begin_ts(begin_ts), c_message_not_a_begin_timestamp);
    ts_entry_t begin_ts_entry = s_txn_info[begin_ts];
    return is_txn_entry_submitted(begin_ts_entry);
}

inline bool server::is_txn_entry_validating(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_validating);
}

inline bool server::is_txn_validating(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    return is_txn_entry_validating(commit_ts_entry);
}

inline bool server::is_txn_entry_decided(ts_entry_t ts_entry)
{
    constexpr uint64_t c_decided_mask = c_txn_status_decided << c_txn_status_flags_shift;
    return ((ts_entry & c_decided_mask) == c_decided_mask);
}

inline bool server::is_txn_decided(gaia_txn_id_t commit_ts)
{
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    retail_assert(is_txn_entry_commit_ts(commit_ts_entry), c_message_not_a_commit_timestamp);
    return is_txn_entry_decided(commit_ts_entry);
}

inline bool server::is_txn_entry_committed(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_committed);
}

inline bool server::is_txn_committed(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    return is_txn_entry_committed(commit_ts_entry);
}

inline bool server::is_txn_entry_aborted(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_aborted);
}

inline bool server::is_txn_aborted(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    return is_txn_entry_aborted(commit_ts_entry);
}

inline bool server::is_txn_entry_gc_complete(ts_entry_t ts_entry)
{
    uint64_t gc_flags = (ts_entry & c_txn_gc_flags_mask) >> c_txn_gc_flags_shift;
    return (gc_flags == c_txn_gc_complete);
}

inline bool server::is_txn_gc_complete(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), "Not a commit timestamp!");
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    return is_txn_entry_gc_complete(commit_ts_entry);
}

inline bool server::is_txn_entry_durable(ts_entry_t ts_entry)
{
    uint64_t persistence_flags = (ts_entry & c_txn_persistence_flags_mask) >> c_txn_persistence_flags_shift;
    return (persistence_flags == c_txn_persistence_complete);
}

inline bool server::is_txn_durable(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), "Not a commit timestamp!");
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    return is_txn_entry_durable(commit_ts_entry);
}

inline bool server::is_txn_entry_active(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_active);
}

inline bool server::is_txn_active(gaia_txn_id_t begin_ts)
{
    retail_assert(begin_ts != c_txn_entry_unknown, c_message_unknown_timestamp);
    retail_assert(is_begin_ts(begin_ts), c_message_not_a_begin_timestamp);
    ts_entry_t ts_entry = s_txn_info[begin_ts];
    return is_txn_entry_active(ts_entry);
}

inline bool server::is_txn_entry_terminated(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_terminated);
}

inline bool server::is_txn_terminated(gaia_txn_id_t begin_ts)
{
    retail_assert(begin_ts != c_txn_entry_unknown, c_message_unknown_timestamp);
    retail_assert(is_begin_ts(begin_ts), c_message_not_a_begin_timestamp);
    ts_entry_t ts_entry = s_txn_info[begin_ts];
    return is_txn_entry_terminated(ts_entry);
}

inline uint64_t server::get_status(gaia_txn_id_t ts)
{
    retail_assert(
        !is_unknown_ts(ts) && !is_invalid_ts(ts),
        "Invalid timestamp entry!");
    ts_entry_t ts_entry = s_txn_info[ts];
    return get_status_from_entry(ts_entry);
}

inline uint64_t server::get_status_from_entry(ts_entry_t ts_entry)
{
    return ((ts_entry & c_txn_status_flags_mask) >> c_txn_status_flags_shift);
}

inline gaia_txn_id_t server::get_begin_ts(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    // The begin_ts is the low 42 bits of the ts entry.
    auto begin_ts = static_cast<gaia_txn_id_t>(commit_ts_entry & c_txn_ts_mask);
    retail_assert(begin_ts != c_invalid_gaia_txn_id, "begin_ts must be valid!");
    return begin_ts;
}

inline gaia_txn_id_t server::get_commit_ts(gaia_txn_id_t begin_ts)
{
    retail_assert(is_begin_ts(begin_ts), c_message_not_a_begin_timestamp);
    retail_assert(is_txn_submitted(begin_ts), "begin_ts must be submitted!");
    ts_entry_t begin_ts_entry = s_txn_info[begin_ts];
    // The commit_ts is the low 42 bits of the begin_ts entry.
    auto commit_ts = static_cast<gaia_txn_id_t>(begin_ts_entry & c_txn_ts_mask);
    if (is_txn_submitted(begin_ts))
    {
        retail_assert(commit_ts != c_invalid_gaia_txn_id, "commit_ts must be valid for a submitted begin_ts!");
    }
    return commit_ts;
}

inline int server::get_txn_log_fd(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);
    return get_txn_log_fd_from_entry(s_txn_info[commit_ts]);
}

inline int server::get_txn_log_fd_from_entry(ts_entry_t commit_ts_entry)
{
    // The txn log fd is the 16 bits of the ts entry after the 3 status bits.
    uint16_t fd = (commit_ts_entry & c_txn_log_fd_mask) >> c_txn_log_fd_shift;
    // If the log fd is invalidated, then return an invalid fd value (-1).
    return (fd != c_invalid_txn_log_fd_bits) ? static_cast<int>(fd) : -1;
}

inline bool server::invalidate_txn_log_fd(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);
    retail_assert(is_txn_decided(commit_ts), "Cannot invalidate an undecided txn!");

    // The txn log fd is the 16 bits of the ts entry after the 3 status bits. We
    // don't zero these out because 0 is technically a valid fd (even though
    // it's normally reserved for stdin). Instead we follow the same convention
    // as elsewhere, using a reserved value of -1 to indicate an invalid fd.
    // (This means, of course, that we cannot use uint16_t::max() as a valid fd.)
    // We need a CAS since only one thread can be allowed to invalidate the fd
    // entry and hence to close the fd.
    // NB: we use compare_exchange_weak() for the global update since we need to
    // retry anyway on concurrent updates, so tolerating spurious failures
    // requires no additional logic.

    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    do
    {
        // NB: commit_ts_entry is an inout argument holding the previous value
        // on failure!
        if (get_txn_log_fd_from_entry(commit_ts_entry) == -1)
        {
            return false;
        }

    } while (!s_txn_info[commit_ts].compare_exchange_weak(
        commit_ts_entry, commit_ts_entry | c_txn_log_fd_mask));

    return true;
}

// Sets the begin_ts entry's status to TXN_SUBMITTED.
inline void server::set_active_txn_submitted(gaia_txn_id_t begin_ts, gaia_txn_id_t commit_ts)
{
    // Only an active txn can be submitted.
    retail_assert(is_txn_active(begin_ts), "Not an active transaction!");

    // Transition the begin_ts entry to the TXN_SUBMITTED state.
    constexpr uint64_t c_submitted_flags = c_txn_status_submitted << c_txn_status_flags_shift;

    // A submitted begin_ts entry has the commit_ts in its low bits.
    ts_entry_t submitted_begin_ts_entry = c_submitted_flags | static_cast<ts_entry_t>(commit_ts);

    // We don't need a CAS here since only the session thread can submit or terminate a txn,
    // and an active txn cannot be invalidated.
    s_txn_info[begin_ts] = submitted_begin_ts_entry;
}

// Sets the begin_ts entry's status to TXN_TERMINATED.
inline void server::set_active_txn_terminated(gaia_txn_id_t begin_ts)
{
    // Only an active txn can be terminated.
    retail_assert(is_txn_active(begin_ts), "Not an active transaction!");

    // We don't need a CAS here since only the session thread can submit or terminate a txn,
    // and an active txn cannot be invalidated.

    // Mask out the existing status flags and mask in the new ones.
    constexpr uint64_t c_terminated_flags = c_txn_status_terminated << c_txn_status_flags_shift;
    ts_entry_t old_entry = s_txn_info[begin_ts];
    ts_entry_t new_entry = c_terminated_flags | (old_entry & ~c_txn_status_flags_mask);
    s_txn_info[begin_ts] = new_entry;
}

inline gaia_txn_id_t server::register_commit_ts(gaia_txn_id_t begin_ts, int log_fd)
{
    // We construct the commit_ts entry by concatenating status flags (3 bits),
    // txn log fd (16 bits), reserved bits (3 bits), and begin_ts (42 bits).
    uint64_t shifted_log_fd = static_cast<uint64_t>(log_fd) << c_txn_log_fd_shift;
    constexpr uint64_t c_shifted_status_flags = c_txn_status_validating << c_txn_status_flags_shift;
    ts_entry_t commit_ts_entry = c_shifted_status_flags | shifted_log_fd | begin_ts;

    // We're possibly racing another beginning or committing txn that wants to
    // invalidate our commit_ts entry. We use compare_exchange_weak() since we
    // need to loop until success anyway. A spurious failure will just waste a
    // timestamp, and the uninitialized entry will eventually be invalidated.
    gaia_txn_id_t commit_ts;
    ts_entry_t expected_entry;
    do
    {
        // Allocate a new commit timestamp.
        commit_ts = allocate_txn_id();
        // The commit_ts entry must fit in 42 bits.
        retail_assert(commit_ts < (1ULL << c_txn_ts_bits), "commit_ts must fit in 42 bits!");
        // The commit_ts entry must be uninitialized.
        expected_entry = c_txn_entry_unknown;

    } while (!s_txn_info[commit_ts].compare_exchange_weak(expected_entry, commit_ts_entry));

    return commit_ts;
}

gaia_txn_id_t server::submit_txn(gaia_txn_id_t begin_ts, int log_fd)
{
    // The begin_ts entry must fit in 42 bits.
    retail_assert(begin_ts < (1ULL << c_txn_ts_bits), "begin_ts must fit in 42 bits!");

    // We assume all our log fds are non-negative and fit into 16 bits. (The
    // highest possible value is reserved to indicate an invalid fd.)
    retail_assert(
        log_fd >= 0 && log_fd < std::numeric_limits<uint16_t>::max(),
        "Transaction log fd is not between 0 and (2^16 - 2)!");

    retail_assert(is_fd_valid(log_fd), "Invalid log fd!");

    // Alocate a new commit_ts and initialize its entry to our begin_ts and log fd.
    gaia_txn_id_t commit_ts = register_commit_ts(begin_ts, log_fd);

    // Now update the active txn entry.
    set_active_txn_submitted(begin_ts, commit_ts);

    return commit_ts;
}

void server::update_txn_decision(gaia_txn_id_t commit_ts, bool committed)
{
    // The commit_ts entry must be in state TXN_VALIDATING or TXN_DECIDED.
    // We allow the latter to enable idempotent concurrent validation.
    retail_assert(
        is_txn_validating(commit_ts) || is_txn_decided(commit_ts),
        "commit_ts entry must be in validating or decided state!");

    uint64_t decided_status_flags = committed ? c_txn_status_committed : c_txn_status_aborted;

    // We can just reuse the log fd and begin_ts from the existing entry.
    ts_entry_t expected_entry = s_txn_info[commit_ts];

    // We may have already been validated by another committing txn.
    if (is_txn_entry_decided(expected_entry))
    {
        // If another txn validated before us, they should have reached the same decision.
        retail_assert(
            is_txn_entry_committed(expected_entry) == committed,
            "Inconsistent txn decision detected!");

        return;
    }

    // It's safe to just OR in the new flags since the preceding states don't set
    // any bits not present in the flags.
    ts_entry_t commit_ts_entry = expected_entry | (decided_status_flags << c_txn_status_flags_shift);
    bool has_set_entry = s_txn_info[commit_ts].compare_exchange_strong(expected_entry, commit_ts_entry);
    if (!has_set_entry)
    {
        // NB: expected_entry is an inout argument holding the previous value on failure!
        retail_assert(
            is_txn_entry_decided(expected_entry),
            "commit_ts entry in validating state can only transition to a decided state!");

        // If another txn validated before us, they should have reached the same decision.
        retail_assert(
            is_txn_entry_committed(expected_entry) == committed,
            "Inconsistent txn decision detected!");

        return;
    }
}

// This helper method takes 2 file descriptors for txn logs and determines if
// they have a non-empty intersection. We could just use std::set_intersection,
// but that outputs all elements of the intersection in a third container, while
// we just need to test for non-empty intersection (and terminate as soon as the
// first common element is found), so we write our own simple merge intersection
// code. If we ever need to return the IDs of all conflicting objects to clients
// (or just log them for diagnostics), we could use std::set_intersection.
bool server::txn_logs_conflict(int log_fd1, int log_fd2)
{
    // First map the two fds.

    txn_log_t* log1;
    map_fd(log1, get_fd_size(log_fd1), PROT_READ, MAP_PRIVATE, log_fd1, 0);
    auto cleanup_log1 = make_scope_guard([&]() {
        unmap_fd(log1, get_fd_size(log_fd1));
    });

    txn_log_t* log2;
    map_fd(log2, get_fd_size(log_fd2), PROT_READ, MAP_PRIVATE, log_fd2, 0);
    auto cleanup_log2 = make_scope_guard([&]() {
        unmap_fd(log2, get_fd_size(log_fd2));
    });

    // Now perform standard merge intersection and terminate on the first conflict found.
    size_t log1_idx = 0, log2_idx = 0;
    while (log1_idx < log1->record_count && log2_idx < log2->record_count)
    {
        txn_log_t::log_record_t* lr1 = log1->log_records + log1_idx;
        txn_log_t::log_record_t* lr2 = log2->log_records + log2_idx;
        if (lr1->locator == lr2->locator)
        {
            return true;
        }
        else if (lr1->locator < lr2->locator)
        {
            ++log1_idx;
        }
        else
        {
            ++log2_idx;
        }
    }
    return false;
}

bool server::validate_txn(gaia_txn_id_t commit_ts)
{
    // Validation algorithm:

    // NB: We make two passes over the conflict window, even though only one
    // pass would suffice, since we want to avoid unnecessary validation of
    // undecided txns by skipping over them on the first pass while we check
    // committed txns for conflicts, hoping that some undecided txns will be
    // decided before the second pass. This adds some complexity (e.g., tracking
    // committed txns already tested for conflicts), but avoids unnecessary
    // duplicated work, by deferring helping other txns validate until all
    // necessary work is complete and we would be forced to block otherwise.
    //
    // 1. Find all committed transactions in conflict window, i.e., between our
    //    begin and commit timestamps, traversing from oldest to newest txns and
    //    testing for conflicts as we go, to allow as much time as possible for
    //    undecided txns to be validated, and for commit timestamps allocated
    //    within our conflict window to be registered. Invalidate all unknown
    //    timestamps we encounter, so no active txns can be submitted within our
    //    conflict window. Any submitted txns which have allocated a commit
    //    timestamp within our conflict window but have not yet registered their
    //    commit timestamp must now retry with a new timestamp. (This allows us
    //    to treat our conflict window as an immutable snapshot of submitted
    //    txns, without needing to acquire any locks.) Skip over all invalidated
    //    timestamps, active txns, undecided txns, and aborted txns. Repeat this
    //    phase until no newly committed txns are discovered.
    //
    // 2. Recursively validate all undecided txns in the conflict window, from
    //    oldest to newest. Note that we cannot recurse indefinitely, since by
    //    hypothesis no undecided txns can precede our begin timestamp (because
    //    a new transaction must validate any undecided transactions with commit
    //    timestamps preceding its begin timestamp before it can proceed).
    //    However, we could duplicate work with other session threads validating
    //    their committing txns. We mitigate this by 1) deferring any recursive
    //    validation until no more committed txns have been found during a
    //    repeated scan of the conflict window, 2) tracking all txns' validation
    //    status in their commit timestamp entries, and 3) rechecking validation
    //    status for the current txn after scanning each possibly conflicting
    //    txn log (and aborting validation if it has already been validated).
    //
    // 3. Test any committed txns validated in the preceding step for conflicts.
    //    (This also includes any txns which were undecided in the first pass
    //    but committed before the second pass.)
    //
    // 4. If any of these steps finds that our write set conflicts with the
    //    write set of any committed txn, we must abort. Otherwise, we commit.
    //    In either case, we set the decided state in our commit timestamp entry
    //    and return the commit decision to the client.
    //
    // REVIEW: Possible optimization for conflict detection, since mmap() is so
    // expensive: store compact signatures in the txn log (either sorted
    // fingerprint sequence or bloom filter, at beginning or end of file), read
    // the signatures with pread(2), and test against signatures in committing
    // txn's log. Only mmap() the txn log if a possible conflict is indicated.
    //
    // REVIEW: A simpler and possibly more important optimization: use the
    // existing mapped view of the committing txn's log instead of mapping it
    // again on every conflict check.
    //
    // REVIEW: Possible optimization (in the original version but removed in the
    // current code for simplicity): find the latest undecided txn which
    // conflicts with our write set. Any undecided txns later than this don't
    // need to be validated (but earlier, non-conflicting undecided txns still
    // need to be validated, since they could conflict with a later undecided
    // txn with a conflicting write set, which would abort and hence not cause
    // us to abort).

    // Since we make multiple passes over the conflict window, we need to track
    // committed txns that have already been tested for conflicts.
    std::unordered_set<gaia_txn_id_t> committed_txns_tested_for_conflicts;

    // Iterate over all txns in conflict window, and test all committed txns for
    // conflicts. Repeat until no new committed txns are discovered. This gives
    // undecided txns as much time as possible to be validated by their
    // committing txn.
    bool new_committed_txn_found;
    do
    {
        new_committed_txn_found = false;
        for (gaia_txn_id_t ts = get_begin_ts(commit_ts) + 1; ts < commit_ts; ++ts)
        {
            // Invalidate all unknown timestamps. This marks a "fence" after which
            // any submitted txns with commit timestamps in our conflict window must
            // already be registered under their commit_ts (they must retry with a
            // new timestamp otherwise). (The invalidation is necessary only on the
            // first pass, but the "unknown timestamp entry" check is cheap enough
            // that repeating it on subsequent passes shouldn't matter.)
            if (invalidate_unknown_ts(ts))
            {
                continue;
            }

            if (is_commit_ts(ts) && is_txn_committed(ts))
            {
                // Remember each committed txn commit_ts so we don't test it again.
                const auto [iter, new_committed_ts] = committed_txns_tested_for_conflicts.insert(ts);
                if (new_committed_ts)
                {
                    new_committed_txn_found = true;

                    // Eagerly test committed txns for conflicts to give undecided
                    // txns more time for validation (and submitted txns more time
                    // for registration).

                    // We need to use the safe_fd_from_ts wrapper for conflict detection
                    // in case either log fd is invalidated by another thread
                    // concurrently advancing the watermark. If either log fd is
                    // invalidated, it must be that another thread has validated our
                    // txn, so we can exit early.
                    try
                    {
                        safe_fd_from_ts validating_fd(commit_ts);
                        safe_fd_from_ts committed_fd(ts);

                        if (txn_logs_conflict(validating_fd.get_fd(), committed_fd.get_fd()))
                        {
                            return false;
                        }
                    }
                    catch (const invalid_log_fd& e)
                    {
                        // invalid_log_fd can only be thrown if the log fd of the
                        // commit_ts it references has been invalidated, which can only
                        // happen if the watermark has advanced past the commit_ts. The
                        // watermark cannot advance past our begin_ts unless our txn has
                        // already been validated, so if either of the fds we are
                        // testing for conflicts is invalidated, it must mean that our
                        // txn has already been validated. Since our commit_ts always
                        // follows the commit_ts of the undecided txn we are testing for
                        // conflicts, and the watermark always advances in order, it
                        // cannot be the case that this txn's log fd has not been
                        // invalidated while our txn's log fd has been invalidated.
                        gaia_txn_id_t invalidated_commit_ts = e.get_ts();
                        retail_assert(
                            is_txn_decided(invalidated_commit_ts),
                            c_message_txn_log_fd_cannot_be_invalidated);
                        if (invalidated_commit_ts == ts)
                        {
                            retail_assert(
                                is_txn_decided(commit_ts),
                                c_message_validating_txn_should_have_been_validated);
                        }
                        // If either log fd was invalidated, then the validating txn
                        // must have been validated, so we can return the decision
                        // immediately.
                        return is_txn_committed(commit_ts);
                    }
                }
            }

            // Check if another thread has already validated this txn.
            if (is_txn_decided(commit_ts))
            {
                return is_txn_committed(commit_ts);
            }
        }

    } while (new_committed_txn_found);

    // Validate all undecided txns, from oldest to newest. If any validated txn
    // commits, test it immediately for conflicts. Also test any committed txns
    // for conflicts if they weren't tested in the first pass.
    for (gaia_txn_id_t ts = get_begin_ts(commit_ts) + 1; ts < commit_ts; ++ts)
    {
        if (is_commit_ts(ts))
        {
            // Validate any currently undecided txn.
            if (is_txn_validating(ts))
            {
                // By hypothesis, there are no undecided txns with commit timestamps
                // preceding the committing txn's begin timestamp.
                retail_assert(
                    ts > s_txn_id,
                    c_message_preceding_txn_should_have_been_validated);

                // Recursively validate the current undecided txn.
                bool committed = validate_txn(ts);

                // Update the current txn's decided status.
                update_txn_decision(ts, committed);
            }

            // If a previously undecided txn has now committed, test it for conflicts.
            if (is_txn_committed(ts) && committed_txns_tested_for_conflicts.count(ts) == 0)
            {
                // We need to use the safe_fd_from_ts wrapper for conflict
                // detection in case either log fd is invalidated by another
                // thread concurrently advancing the watermark. If either log fd
                // is invalidated, it must be that another thread has validated
                // our txn, so we can exit early.
                try
                {
                    safe_fd_from_ts validating_fd(commit_ts);
                    safe_fd_from_ts new_committed_fd(ts);

                    if (txn_logs_conflict(validating_fd.get_fd(), new_committed_fd.get_fd()))
                    {
                        return false;
                    }
                }
                catch (const invalid_log_fd& e)
                {
                    // invalid_log_fd can only be thrown if the log fd of the
                    // commit_ts it references has been invalidated, which can
                    // only happen if the watermark has advanced past the
                    // commit_ts. The watermark cannot advance past our begin_ts
                    // unless our txn has already been validated, so if either
                    // of the fds we are testing for conflicts is invalidated,
                    // it must mean that our txn has already been validated.
                    // Since our commit_ts always follows the commit_ts of the
                    // undecided txn we are testing for conflicts, and the
                    // watermark always advances in order, it cannot be the case
                    // that this txn's log fd has not been invalidated while our
                    // txn's log fd has been invalidated.
                    gaia_txn_id_t invalidated_commit_ts = e.get_ts();
                    retail_assert(
                        is_txn_decided(invalidated_commit_ts),
                        c_message_txn_log_fd_cannot_be_invalidated);
                    if (invalidated_commit_ts == commit_ts)
                    {
                        retail_assert(
                            get_txn_log_fd(ts) == -1,
                            c_message_txn_log_fd_should_have_been_invalidated);
                    }
                    else
                    {
                        retail_assert(
                            invalidated_commit_ts == ts,
                            c_message_unexpected_commit_ts_value);
                        retail_assert(
                            is_txn_decided(commit_ts),
                            c_message_validating_txn_should_have_been_validated);
                    }
                    // If either log fd was invalidated, then the validating txn
                    // must have been validated, so we can return the decision
                    // immediately.
                    return is_txn_committed(commit_ts);
                }
            }
        }

        // Check if another thread has already validated this txn.
        if (is_txn_decided(commit_ts))
        {
            return is_txn_committed(commit_ts);
        }
    }

    // At this point, there are no undecided txns in the conflict window, and
    // all committed txns have been tested for conflicts, so we can commit.
    return true;
}

// This advances the given global timestamp counter to the given timestamp
// value, unless it has already advanced beyond the given value due to a
// concurrent update.
//
// NB: we use compare_exchange_weak() for the global update since we need to
// retry anyway on concurrent updates, so tolerating spurious failures
// requires no additional logic.
bool server::advance_watermark(std::atomic<gaia_txn_id_t>& watermark, gaia_txn_id_t ts)
{
    gaia_txn_id_t last_watermark_ts = watermark;
    do
    {
        // NB: last_watermark_ts is an inout argument holding the previous value
        // on failure!
        if (ts <= last_watermark_ts)
        {
            return false;
        }

    } while (!watermark.compare_exchange_weak(last_watermark_ts, ts));

    return true;
}

void server::apply_txn_log_from_ts(gaia_txn_id_t commit_ts)
{
    retail_assert(
        is_commit_ts(commit_ts) && is_txn_committed(commit_ts),
        "apply_txn_log_from_ts() must be called on the commit_ts of a committed txn!");

    // Since txn logs are only eligible for GC after they fall behind the
    // post-apply watermark, we don't need the safe_fd_from_ts wrapper.
    int log_fd = get_txn_log_fd(commit_ts);

    // A txn log fd should never be invalidated until it falls behind the
    // post-apply watermark.
    retail_assert(
        log_fd != -1,
        "apply_txn_log_from_ts() must be called on a commit_ts with a valid log fd!");

    txn_log_t* txn_log;
    map_fd(txn_log, get_fd_size(log_fd), PROT_READ, MAP_PRIVATE, log_fd, 0);
    // Ensure the fd is unmapped when we exit this scope.
    auto cleanup_log_mapping = make_scope_guard([&]() {
        unmap_fd(txn_log, txn_log->size());
    });

    // Ensure that the begin_ts in this entry matches the txn log header.
    retail_assert(
        txn_log->begin_ts == get_begin_ts(commit_ts),
        "txn log begin_ts must match begin_ts reference in commit_ts entry!");

    for (size_t i = 0; i < txn_log->record_count; ++i)
    {
        // Update the shared locator view with each redo version (i.e., the
        // version created or updated by the txn). This is safe as long as the
        // committed txn being applied has commit_ts older than the oldest
        // active txn's begin_ts (so it can't overwrite any versions visible in
        // that txn's snapshot). This update is non-atomic since log application
        // is idempotent and therefore a txn log can be re-applied over the same
        // txn's partially-applied log during snapshot reconstruction.
        txn_log_t::log_record_t* lr = &(txn_log->log_records[i]);
        (*s_shared_locators)[lr->locator] = lr->new_offset;
    }

    // We're using the otherwise-unused first entry of the "locators" array to
    // track the last-applied commit_ts (purely for diagnostic purposes).
    bool updated_locators_view_version = advance_watermark((*s_shared_locators)[0], commit_ts);
    retail_assert(
        updated_locators_view_version,
        "Committed txn applied to shared locators view out of order!");
}

bool server::set_txn_gc_complete(gaia_txn_id_t commit_ts)
{
    ts_entry_t expected_entry = s_txn_info[commit_ts];
    // Set GC status to TXN_GC_COMPLETE.
    ts_entry_t commit_ts_entry = expected_entry | (c_txn_gc_complete << c_txn_gc_flags_shift);
    return s_txn_info[commit_ts].compare_exchange_strong(expected_entry, commit_ts_entry);
}

void server::gc_txn_log_from_fd(int log_fd, bool committed)
{
    txn_log_t* txn_log;
    map_fd(txn_log, get_fd_size(log_fd), PROT_READ, MAP_PRIVATE, log_fd, 0);
    // Ensure the fd is unmapped when we exit this scope.
    auto cleanup_log_mapping = make_scope_guard([&]() {
        unmap_fd(txn_log, txn_log->size());
    });

    bool deallocate_new_offsets = !committed;
    deallocate_txn_log(txn_log, deallocate_new_offsets);
}

void server::deallocate_txn_log(txn_log_t* txn_log, bool deallocate_new_offsets)
{
    retail_assert(txn_log, "txn_log must be a valid mapped address!");

    for (size_t i = 0; i < txn_log->record_count; ++i)
    {
        // For committed txns, free each undo version (i.e., the version
        // superseded by an update or delete operation), using the registered
        // object deallocator (if it exists), since the redo versions may still
        // be visible but the undo versions cannot be. For aborted or
        // rolled-back txns, free only the redo versions (since the undo
        // versions may still be visible).
        // NB: we can't safely free the redo versions and txn logs of aborted
        // txns in the decide handler, because concurrently validating txns
        // might be testing the aborted txn for conflicts while they still think
        // it is undecided. The only safe place to free the redo versions and
        // txn log of an aborted txn is after it falls behind the watermark,
        // since at that point it cannot be in the conflict window of any
        // committing txn.
        gaia_offset_t offset_to_free = deallocate_new_offsets
            ? txn_log->log_records[i].new_offset
            : txn_log->log_records[i].old_offset;

        if (offset_to_free && s_object_deallocator_fn)
        {
            s_object_deallocator_fn(offset_to_free);
        }
    }
}

// This method is called by an active txn when it is terminated or by a
// submitted txn after it is validated. It performs a few system maintenance
// tasks, which can be deferred indefinitely with no effect on availability or
// correctness, but which are essential to maintain acceptable performance and
// resource usage.
//
// To enable reasoning about the safety of reclaiming resources which should no
// longer be needed by any present or future txns, and other invariant-based
// reasoning, we define a set of "watermarks": upper or lower bounds on the
// endpoint of a sequence of txns with some property. There are currently three
// watermarks defined: the "pre-apply" watermark, which serves as an upper bound
// on the last committed txn which was fully applied to the shared view; the
// "post-apply" watermark, which serves as a lower bound on the last committed
// txn which was fully applied to the shared view; and the "post-GC" watermark,
// which serves as a lower bound on the last txn to have its resources fully
// reclaimed (i.e., its txn log and all its undo or redo versions deallocated,
// for a committed or aborted txn respectively). The post-GC watermark is
// currently unused, but will be used to implement trimming the txn table (i.e.,
// deallocating physical pages corresponding to virtual address space that will
// never be read or written again).
//
// At a high level, the first pass applies all committed txn logs to the shared
// view, in order (not concurrently), and advances two watermarks marking an
// upper bound and lower bound respectively on the timestamp of the latest txn
// whose redo log has been completely applied to the shared view. The second
// pass executes GC operations concurrently on all txns which have either
// aborted or been fully applied to the shared view (and have been durably
// logged if persistence is enabled), and sets a flag on each txn when GC is
// complete. The third pass simply advances a watermark to the latest txn for
// which GC has completed for it and all its predecessors, marking a lower bound
// on the oldest txn whose metadata cannot yet be safely reclaimed.
//
// 1. We scan the interval from a snapshot of the pre-apply watermark to a
//    snapshot of the last allocated timestamp, and attempt to advance the
//    pre-apply watermark if it has the same value as the post-apply watermark,
//    and if the next timestamp entry is either an invalid timestamp (we
//    invalidate all unknown entries during the scan), a commit_ts that is
//    decided, or a begin_ts that is terminated or submitted with a decided
//    commit_ts. If we successfully advanced the watermark and the current entry
//    is a committed commit_ts, then we can apply its redo log to the shared
//    view. After applying it (or immediately after advancing the pre-apply
//    watermark if the current timestamp is not a comoitted commit_ts), we
//    advance the post-apply watermark to the same timestamp. (Since we "own"
//    the current timestamp entry after a successful CAS on the pre-apply
//    watermark, we can advance the post-apply watermark without a CAS.) Since
//    the pre-apply watermark can only move forward, updates to the shared view
//    are applied in timestamp order, and since the pre-apply watermark can only
//    be advanced if the post-apply watermark has caught up with it (which can
//    only be the case for a committed commit_ts if the redo log has been fully
//    applied), updates to the shared view are never applied concurrently.
//
// 2. We scan the interval from a snapshot of the post-GC watermark to a
//    snapshot of the post-apply watermark. If the current timestamp is not a
//    commit_ts, we continue the scan. Otherwise we check if its log fd is
//    invalidated. If so, then we know that GC is in progress or complete, so we
//    continue the scan. If persistence is enabled then we also check the
//    durable flag on the current timestamp entry and abort the scan if it is
//    not set (to avoid freeing any redo versions that are being applied to the
//    write-ahead log). Otherwise we try to invalidate its log fd. If
//    invalidation fails, we abort the pass to avoid contention, otherwise we GC
//    this txn using the invalidated log fd, set the TXN_GC_COMPLETE flag, and
//    continue the scan.
//
// 3. Again, we scan the interval from a snapshot of the post-GC watermark to a
//    snapshot of the post-apply watermark. If the current entry is a begin_ts
//    or a commit_ts with TXN_GC_COMPLETE set, we try to advance the post-GC
//    watermark. If that fails (or the watermark cannot be advanced because the
//    commit_ts has TXN_GC_COMPLETE unset), we abort the pass.
//
// TODO: Deallocate physical pages backing the txn table for addresses preceding
// the post-GC watermark (via madvise(MADV_FREE)).
//
// The post-GC watermark will be used to trim the txn table (but we can't just
// read the current value and free all pages behind it, we need to avoid freeing
// pages out from under threads that are advancing this watermark and have
// possibly been preempted). A safe algorithm requires tracking the last
// observed value of this watermark for each thread in a global table and
// trimming the txn table up to the minimum of this global table (which can be
// calculated from a non-atomic scan, since any thread's observed value can only
// go forward). This means that a thread must register its observed value in the
// global table every time it reads the post-GC watermark, and should clear its
// entry when it's done with the observed value. (A subtler issue is that
// reading the current watermark value and registering the read value in the
// global table must be an atomic operation for the non-atomic minimum scan to
// be correct, but no CPU has an atomic memory-to-memory copy operation. We can
// work around this by just checking the current value of the watermark
// immediately after writing its saved value to the global table. If they're the
// same, then no other thread could have observed any non-atomicity in the copy
// operation. If they differ, then we just repeat the sequence. Since this is a
// very fast sequence of operations, retries should be infrequent, so livelock
// shouldn't be an issue.)

void server::perform_maintenance()
{
    // Attempt to apply all txn logs to the shared view, from the last value of
    // the post-apply watermark to the latest committed txn.
    apply_txn_logs_to_shared_view();

    // Attempt to reclaim the resources of all txns from the post-GC watermark
    // to the post-apply watermark.
    gc_applied_txn_logs();

    // Advance the post-GC watermark to the end of the longest prefix of
    // committed txns whose resources have been completely reclaimed, to mark a
    // safe point for truncating transaction history.
    update_txn_table_safe_truncation_point();
}

void server::apply_txn_logs_to_shared_view()
{
    // First get a snapshot of the timestamp counter for an upper bound on
    // the scan (we don't know yet if this is a begin_ts or commit_ts).
    gaia_txn_id_t last_allocated_ts = get_last_txn_id();

    // Now get a snapshot of the last_applied_commit_ts_upper_bound watermark,
    // for a lower bound on the scan.
    gaia_txn_id_t last_applied_commit_ts_upper_bound = s_last_applied_commit_ts_upper_bound;

    // Scan from the saved pre-apply watermark to the last known
    // timestamp, to find the oldest active txn (if any) after begin_ts and the
    // newest committed txn (if any) preceding the oldest active txn if it
    // exists, or before the last known timestamp otherwise, and apply all
    // committed txn logs in the scan interval to the shared view.

    for (gaia_txn_id_t ts = last_applied_commit_ts_upper_bound + 1; ts <= last_allocated_ts; ++ts)
    {
        // We need to invalidate unknown entries as we go along, so that we
        // don't miss any active begin_ts or committed commit_ts entries.
        invalidate_unknown_ts(ts);

        // If this is a commit_ts, we cannot advance the watermark unless it's
        // decided.
        if (is_commit_ts(ts) && is_txn_validating(ts))
        {
            if (is_txn_validating(ts))
            {
                break;
            }
        }

        // The watermark cannot be advanced past any begin_ts whose txn is not
        // either in the TXN_TERMINATED state or in the TXN_SUBMITTED state with
        // its commit_ts in the TXN_DECIDED state. This means that the watermark
        // can never advance into the conflict window of an undecided txn.
        if (is_begin_ts(ts))
        {
            if (is_txn_active(ts))
            {
                break;
            }

            if (is_txn_submitted(ts) && is_txn_validating(get_commit_ts(ts)))
            {
                break;
            }
        }

        // We can only advance the pre-apply watermark if the post-apply
        // watermark has caught up to it (this ensures that txn logs cannot be
        // applied concurrently to the shared view; they are already applied in
        // order because the pre-apply watermark advances in order). This is
        // exactly equivalent to a lock implemented by a CAS attempting to set a
        // boolean. When a thread successfully advances the pre-apply watermark,
        // it acquires the lock (because this makes the pre-apply and post-apply
        // watermarks unequal, so no other thread will attempt to advance the
        // pre-apply watermark, and a thread can only advance the post-apply
        // watermark after advancing the pre-apply watermark), and when the
        // owning thread subsequently advances the post-apply watermark, it
        // releases the lock (because all other threads observe that the
        // pre-apply and post-apply watermarks are equal again and can attempt
        // to advance the pre-apply watermark). This "inchworm" algorithm
        // (so-called because like an inchworm, the "front" can only advance
        // after the "back" has caught up) is thus operationally equivalent to
        // locking on a reserved bit flag (call it TXN_GC_ELIGIBLE) in the
        // timestamp entry, but allows us to avoid reserving another scarce bit
        // for this purpose.

        // The current timestamp in the scan is guaranteed to be positive due to the loop precondition.
        gaia_txn_id_t prev_ts = ts - 1;
        // This thread must have observed both the pre- and post-apply watermarks to be equal to the previous timestamp
        // in the scan in order to advance the pre-apply watermark to the current timestamp in the scan. This means that
        // any thread applying a txn log at the previous timestamp must have finished applying the log, so we can safely
        // apply the log at the current timestamp.
        // REVIEW: These loads could be relaxed, since a stale read could only result in premature abort of the scan.
        if (s_last_applied_commit_ts_upper_bound != prev_ts || s_last_applied_commit_ts_lower_bound != prev_ts)
        {
            break;
        }

        if (!advance_watermark(s_last_applied_commit_ts_upper_bound, ts))
        {
            // If another thread has already advanced the watermark ahead of
            // this ts, we abort advancing it further.
            retail_assert(
                s_last_applied_commit_ts_upper_bound > last_applied_commit_ts_upper_bound,
                "The watermark must have advanced if advance_watermark failed!");

            break;
        }

        if (is_commit_ts(ts))
        {
            retail_assert(is_txn_decided(ts), "The watermark should not be advanced to an undecided commit_ts!");

            if (is_txn_committed(ts))
            {
                // If a new txn starts after or while we apply this txn log to
                // the shared view, but before we advance the post-apply
                // watermark, it will re-apply some of our updates to its
                // snapshot of the shared view, but that is benign since log
                // replay is idempotent (as long as logs are applied in
                // timestamp order).
                apply_txn_log_from_ts(ts);
            }
        }

        // Now we advance the post-apply watermark to catch up with the pre-apply watermark.
        // REVIEW: Since no other thread can concurrently advance the post-apply watermark,
        // we don't need a full CAS here.
        bool has_advanced_watermark = advance_watermark(s_last_applied_commit_ts_lower_bound, ts);

        // No other thread should be able to advance the post-apply watermark,
        // since only one thread can advance the pre-apply watermark to this
        // timestamp.
        retail_assert(has_advanced_watermark, "Couldn't advance the post-apply watermark!");
    }
}

void server::gc_applied_txn_logs()
{
    // First get a snapshot of the post-apply watermark, for an upper bound on the scan.
    gaia_txn_id_t last_freed_commit_ts_lower_bound = s_last_freed_commit_ts_lower_bound;

    // Now get a snapshot of the post-GC watermark, for a lower bound on the scan.
    gaia_txn_id_t last_applied_commit_ts_lower_bound = s_last_applied_commit_ts_lower_bound;

    // Scan from the post-GC watermark to the post-apply watermark,
    // executing GC on any commit_ts if the log fd is valid (and the durable
    // flag is set if persistence is enabled). (If we fail to invalidate the log
    // fd, we abort the scan to avoid contention.) When GC is complete, set the
    // TXN_GC_COMPLETE flag on the txn entry and continue.

    for (gaia_txn_id_t ts = last_freed_commit_ts_lower_bound + 1; ts <= last_applied_commit_ts_lower_bound; ++ts)
    {
        retail_assert(!is_unknown_ts(ts), "All unknown txn table entries should be invalidated!");

        retail_assert(
            !(is_begin_ts(ts) && is_txn_active(ts)),
            "The watermark should not be advanced to an active begin_ts!");

        if (is_commit_ts(ts))
        {
            // If persistence is enabled, then we also need to check that
            // TXN_PERSISTENCE_COMPLETE is set (to avoid having redo versions
            // deallocated while they're being persisted).
            bool is_persistence_enabled = (s_persistence_mode != persistence_mode_t::e_disabled)
                && (s_persistence_mode != persistence_mode_t::e_disabled_after_recovery);

            if (is_persistence_enabled && !is_txn_durable(ts))
            {
                break;
            }

            int log_fd = get_txn_log_fd(ts);

            // Continue the scan if this log fd has already been invalidated,
            // since GC has already been started.
            if (log_fd == -1)
            {
                continue;
            }

            // Invalidate the log fd, GC the obsolete versions in the log, and close the fd.

            // Abort the scan when invalidation fails to avoid contention.
            if (!invalidate_txn_log_fd(ts))
            {
                break;
            }

            // The log fd can't be closed until after it has been invalidated.
            retail_assert(is_fd_valid(log_fd), "The log fd cannot be closed if we successfully invalidated it!");

            // Since we invalidated the log fd, we now hold the only accessible
            // copy of the fd, so we need to ensure it is closed.
            auto cleanup_fd = make_scope_guard([&]() { close_fd(log_fd); });

            // If the txn committed, we deallocate only undo versions, since the
            // redo versions may still be visible after the txn has fallen
            // behind the watermark. If the txn aborted, then we deallocate only
            // redo versions, since the undo versions may still be visible. Note
            // that we could deallocate intermediate versions (i.e., those
            // superseded within the same txn) immediately, but we do it here
            // for simplicity.
            gc_txn_log_from_fd(log_fd, is_txn_committed(ts));

            // We need to mark this txn entry TXN_GC_COMPLETE to allow the
            // post-GC watermark to advance.
            bool has_set_entry = set_txn_gc_complete(ts);

            // If persistence is enabled, then this commit_ts must have been
            // marked durable before we advanced the watermark, and no other
            // thread can set TXN_GC_COMPLETE after we invalidate the log fd, so
            // it should not be possible for this CAS to fail.
            retail_assert(has_set_entry, "This txn entry cannot change after we invalidate the log fd!");
        }
    }
}

void server::update_txn_table_safe_truncation_point()
{
    // First get a snapshot of the post-apply watermark, for an upper bound on the scan.
    gaia_txn_id_t last_applied_commit_ts_lower_bound = s_last_applied_commit_ts_lower_bound;

    // Now get a snapshot of the post-GC watermark, for a lower bound on the scan.
    gaia_txn_id_t last_freed_commit_ts_lower_bound = s_last_freed_commit_ts_lower_bound;

    // Scan from the post-GC watermark to the post-apply watermark,
    // advancing the post-GC watermark on any begin_ts, or any commit_ts that
    // has the TXN_GC_COMPLETE flag set. If TXN_GC_COMPLETE is unset on the
    // current commit_ts, abort the scan.

    for (gaia_txn_id_t ts = last_freed_commit_ts_lower_bound + 1; ts <= last_applied_commit_ts_lower_bound; ++ts)
    {
        retail_assert(!is_unknown_ts(ts), "All unknown txn table entries should be invalidated!");

        retail_assert(
            !(is_begin_ts(ts) && is_txn_active(ts)),
            "The watermark should not be advanced to an active begin_ts!");

        if (is_commit_ts(ts))
        {
            retail_assert(is_txn_decided(ts), "The watermark should not be advanced to an undecided commit_ts!");

            // We can only advance the post-GC watermark to a commit_ts if it is
            // marked TXN_GC_COMPLETE.
            if (!is_txn_gc_complete(ts))
            {
                break;
            }
        }

        if (!advance_watermark(s_last_freed_commit_ts_lower_bound, ts))
        {
            // If another thread has already advanced the post-GC watermark
            // ahead of this ts, we abort advancing it further.
            retail_assert(
                s_last_freed_commit_ts_lower_bound > last_freed_commit_ts_lower_bound,
                "The watermark must have advanced if advance_watermark failed!");

            break;
        }

        // There are no actions to take after advancing the post-GC watermark,
        // since the post-GC watermark only exists to provide a safe lower
        // bound for truncating transaction history.
    }
}

// This method allocates a new begin_ts and initializes its entry.
gaia_txn_id_t server::txn_begin()
{
    // Allocate a new begin timestamp.
    gaia_txn_id_t begin_ts = allocate_txn_id();

    // The begin_ts entry must fit in 42 bits.
    retail_assert(begin_ts < (1ULL << c_txn_ts_bits), "begin_ts must fit in 42 bits!");

    // The begin_ts entry must have its status initialized to TXN_ACTIVE.
    // All other bits should be 0.
    constexpr ts_entry_t c_begin_ts_entry = c_txn_status_active << c_txn_status_flags_shift;

    // We're possibly racing another beginning or committing txn that wants to
    // invalidate our begin_ts entry.
    ts_entry_t expected_entry = c_txn_entry_unknown;
    bool has_set_entry = s_txn_info[begin_ts].compare_exchange_strong(expected_entry, c_begin_ts_entry);

    // Only the txn thread can transition its begin_ts entry from
    // c_txn_entry_unknown to any state except c_txn_entry_invalid.
    if (!has_set_entry)
    {
        // NB: expected_entry is an inout argument holding the previous value on failure!
        retail_assert(
            expected_entry == c_txn_entry_invalid,
            "Only an invalid value can be set on an empty begin_ts entry by another thread!");

        // Return c_invalid_gaia_txn_id to indicate failure. The caller can
        // retry with a new timestamp.
        // REVIEW: should we do this loop internally using compare_exchange_weak
        // and allocate a new begin_ts on each retry?
        begin_ts = c_invalid_gaia_txn_id;
    }

    return begin_ts;
}

void server::txn_rollback()
{
    retail_assert(
        s_txn_id != c_invalid_gaia_txn_id,
        "txn_rollback() was called without an active transaction!");

    auto cleanup_log_fd = make_scope_guard([&]() {
        close_fd(s_fd_log);
    });

    // Set our txn status to TXN_TERMINATED.
    // NB: this must be done before calling perform_maintenance()!
    set_active_txn_terminated(s_txn_id);

    // Update watermarks and perform associated maintenance tasks.
    perform_maintenance();

    // This session now has no active txn.
    s_txn_id = c_invalid_gaia_txn_id;

    if (s_fd_log != -1)
    {
        // Free any deallocated objects.
        gc_txn_log_from_fd(s_fd_log, false);
    }
}

// Before this method is called, we have already received the log fd from the client
// and mmapped it.
// This method returns true for a commit decision and false for an abort decision.
bool server::txn_commit()
{
    retail_assert(s_fd_log != -1, c_message_uninitialized_fd_log);

    // Register the committing txn under a new commit timestamp.
    gaia_txn_id_t commit_ts = submit_txn(s_txn_id, s_fd_log);

    // This is only used for persistence.
    std::string txn_name;

    if (rdb)
    {
        txn_name = rdb->begin_txn(s_txn_id);
        // Prepare log for transaction.
        rdb->prepare_wal_for_write(s_log, txn_name);
    }

    // Validate the txn against all other committed txns in the conflict window.
    retail_assert(s_fd_log != -1, c_message_uninitialized_fd_log);

    // Validate the committing txn.
    bool committed = validate_txn(commit_ts);

    // Update the txn entry with our commit decision.
    update_txn_decision(commit_ts, committed);

    // Append commit or rollback marker to the WAL.
    if (rdb)
    {
        if (committed)
        {
            rdb->append_wal_commit_marker(txn_name);
        }
        else
        {
            rdb->append_wal_rollback_marker(txn_name);
        }
    }

    return committed;
}

// this must be run on main thread
// see https://thomastrapp.com/blog/signal-handler-for-multithreaded-c++/
void server::run(persistence_mode_t persistence_mode)
{
    // There can only be one thread running at this point, so this doesn't need synchronization.
    s_persistence_mode = persistence_mode;

    // Block handled signals in this thread and subsequently spawned threads.
    sigset_t handled_signals = mask_signals();

    while (true)
    {
        // Create eventfd shutdown event.
        s_server_shutdown_eventfd = make_eventfd();
        auto cleanup_shutdown_eventfd = make_scope_guard([]() {
            // We can't close this fd until all readers and writers have exited.
            // The only readers are the client dispatch thread and the session
            // threads, and the only writer is the signal handler thread. All
            // these threads must have exited before we exit this scope and this
            // handler executes.
            close_fd(s_server_shutdown_eventfd);
        });

        // Launch signal handler thread.
        int caught_signal = 0;
        std::thread signal_handler_thread(signal_handler, handled_signals, std::ref(caught_signal));

        // Initialize global data structures.
        init_shared_memory();
        init_txn_info();

        // Initialize watermarks.
        s_last_applied_commit_ts_upper_bound = c_invalid_gaia_txn_id;
        s_last_applied_commit_ts_lower_bound = c_invalid_gaia_txn_id;
        s_last_freed_commit_ts_lower_bound = c_invalid_gaia_txn_id;

        // Launch thread to listen for client connections and create session threads.
        std::thread client_dispatch_thread(client_dispatch_handler);

        // The client dispatch thread will only return after all sessions have been disconnected
        // and the listening socket has been closed.
        client_dispatch_thread.join();

        // The signal handler thread will only return after a blocked signal is pending.
        signal_handler_thread.join();

        // We shouldn't get here unless the signal handler thread has caught a signal.
        retail_assert(caught_signal != 0, "A signal should have been caught!");

        // We special-case SIGHUP to force reinitialization of the server.
        // This is only enabled if persistence is disabled, because otherwise
        // data would disappear on reset, only to reappear when the database is
        // restarted and recovers from the persistent store.
        if (!(caught_signal == SIGHUP
              && (s_persistence_mode == persistence_mode_t::e_disabled
                  || s_persistence_mode == persistence_mode_t::e_disabled_after_recovery)))
        {
            if (caught_signal == SIGHUP)
            {
                cerr << "Unable to reset the server because persistence is enabled, exiting." << endl;
            }

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
