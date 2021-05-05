/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_server.hpp"

#include <unistd.h>

#include <csignal>

#include <algorithm>
#include <atomic>
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

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/memory_allocation_error.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/common/socket_helpers.hpp"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "messages_generated.h"
#include "persistent_store_manager.hpp"
#include "record_list_manager.hpp"
#include "txn_metadata.hpp"

using namespace std;

using namespace flatbuffers;
using namespace gaia::db;
using namespace gaia::db::messages;
using namespace gaia::db::memory_manager;
using namespace gaia::db::storage;
using namespace gaia::common;
using namespace gaia::common::iterators;
using namespace gaia::common::scope_guard;

static constexpr char c_message_uninitialized_fd_log[] = "Uninitialized fd log!";
static constexpr char c_message_unexpected_event_received[] = "Unexpected event received!";
static constexpr char c_message_current_event_is_inconsistent_with_state_transition[]
    = "Current event is inconsistent with state transition!";
static constexpr char c_message_unexpected_request_data_type[] = "Unexpected request data type!";
static constexpr char c_message_thread_must_be_joinable[] = "Thread must be joinable!";
static constexpr char c_message_epoll_create1_failed[] = "epoll_create1() failed!";
static constexpr char c_message_epoll_wait_failed[] = "epoll_wait() failed!";
static constexpr char c_message_epoll_ctl_failed[] = "epoll_ctl() failed!";
static constexpr char c_message_unexpected_event_type[] = "Unexpected event type!";
static constexpr char c_message_epollerr_flag_should_not_be_set[] = "EPOLLERR flag should not be set!";
static constexpr char c_message_unexpected_fd[] = "Unexpected fd!";
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

server_t::safe_fd_from_ts_t::safe_fd_from_ts_t(gaia_txn_id_t commit_ts, bool auto_close_fd)
    : m_auto_close_fd(auto_close_fd)
{
    ASSERT_PRECONDITION(
        txn_metadata_t::is_commit_ts(commit_ts),
        "You must initialize safe_fd_from_ts_t from a valid commit_ts!");

    // If the log fd was invalidated, it is either closed or soon will
    // be closed, and therefore we cannot use it. We return early if the
    // fd has already been invalidated to avoid the dup(2) call.
    int log_fd = txn_metadata_t::get_txn_log_fd(commit_ts);
    if (log_fd == -1)
    {
        throw invalid_log_fd(commit_ts);
    }

    // To avoid races from the log fd being closed out from under us by a
    // concurrent updater, we first dup(2) the fd, and then if the dup was
    // successful, test the log fd entry again to ensure we aren't reusing a
    // closed log fd. If the log fd was invalidated, then we need to close our
    // dup fd and return false.
    try
    {
        m_local_log_fd = common::duplicate_fd(log_fd);
    }
    catch (const common::system_error& e)
    {
        // The log fd was already closed by another thread (after being
        // invalidated).
        // NB: This does not handle the case of the log fd being closed and then
        // reused before our dup(2) call. That case is handled below, where we
        // check for invalidation.
        if (e.get_errno() == EBADF)
        {
            // The log fd must have been invalidated before it was closed.
            ASSERT_INVARIANT(
                txn_metadata_t::get_txn_log_fd(commit_ts) == -1,
                "log fd was closed without being invalidated!");

            // We lost the race because the log fd was invalidated and closed after our check.
            throw invalid_log_fd(commit_ts);
        }
        else
        {
            throw;
        }
    }

    // If we were able to duplicate the log fd, check to be sure it
    // wasn't invalidated (and thus possibly closed and reused before
    // the call to dup(2)), so we know we aren't reusing a closed fd.
    if (txn_metadata_t::get_txn_log_fd(commit_ts) == -1)
    {
        // If we got here, we must have a valid dup fd.
        ASSERT_INVARIANT(
            common::is_fd_valid(m_local_log_fd),
            "fd should be valid if dup() succeeded!");

        // We need to close the duplicated fd because the original fd
        // might have been reused and we would leak it otherwise
        // (because the destructor isn't called if the constructor
        // throws).
        common::close_fd(m_local_log_fd);
        throw invalid_log_fd(commit_ts);
    }
}

server_t::safe_fd_from_ts_t::~safe_fd_from_ts_t()
{
    // Ensure we close the dup log fd. If the original log fd was closed
    // already (indicated by get_txn_log_fd() returning -1), this will free
    // the shared-memory object referenced by the fd.
    if (m_auto_close_fd)
    {
        // If the constructor fails, this will handle an invalid fd (-1)
        // correctly.
        common::close_fd(m_local_log_fd);
    }
}

int server_t::safe_fd_from_ts_t::get_fd() const
{
    return m_local_log_fd;
}

// This assignment is non-atomic because there seems to be no reason to expect concurrent invocations.
void server_t::register_object_deallocator(std::function<void(gaia_offset_t)> deallocator_fn)
{
    s_object_deallocator_fn = deallocator_fn;
}

void server_t::handle_connect(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    ASSERT_PRECONDITION(event == session_event_t::CONNECT, c_message_unexpected_event_received);

    // This message should only be received after the client thread was first initialized.
    ASSERT_PRECONDITION(
        old_state == session_state_t::DISCONNECTED && new_state == session_state_t::CONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    // We need to reply to the client with the fds for the data/locator segments.
    FlatBufferBuilder builder;
    build_server_reply(builder, session_event_t::CONNECT, old_state, new_state);

    int send_fds[] = {s_shared_locators.fd(), s_shared_counters.fd(), s_shared_data.fd(), s_shared_id_index.fd()};
    send_msg_with_fds(s_session_socket, send_fds, std::size(send_fds), builder.GetBufferPointer(), builder.GetSize());
}

void server_t::handle_begin_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    ASSERT_PRECONDITION(event == session_event_t::BEGIN_TXN, c_message_unexpected_event_received);

    // This message should only be received while a transaction is in progress.
    ASSERT_PRECONDITION(
        old_state == session_state_t::CONNECTED && new_state == session_state_t::TXN_IN_PROGRESS,
        c_message_current_event_is_inconsistent_with_state_transition);

    ASSERT_PRECONDITION(s_txn_id == c_invalid_gaia_txn_id, "Transaction begin timestamp should be uninitialized!");

    ASSERT_PRECONDITION(s_fd_log == -1, "Transaction log fd should be uninitialized!");

    // Allocate a new begin_ts for this txn and initialize its metadata in the txn table.
    s_txn_id = txn_metadata_t::txn_begin();

    // The begin_ts returned by txn_begin() should always be valid because it
    // retries on concurrent invalidation.
    ASSERT_INVARIANT(s_txn_id != c_invalid_gaia_txn_id, "Begin timestamp is invalid!");

    // Ensure that there are no undecided txns in our snapshot window.
    validate_txns_in_range(s_last_applied_commit_ts_upper_bound + 1, s_txn_id);

    // REVIEW: we could make this a session thread-local to avoid dynamic
    // allocation per txn.
    std::vector<int> txn_log_fds;
    get_txn_log_fds_for_snapshot(s_txn_id, txn_log_fds);

    // Send the reply message to the client, with the number of txn log fds to
    // be sent later.
    // REVIEW: We could optimize the fast path by piggybacking some small fixed
    // number of log fds on the initial reply message.
    FlatBufferBuilder builder;
    build_server_reply(builder, session_event_t::BEGIN_TXN, old_state, new_state, s_txn_id, txn_log_fds.size());
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Send all txn log fds to the client in an additional sequence of dummy messages.
    // We need a 1-byte dummy message buffer due to our datagram size convention.
    uint8_t msg_buf[1] = {0};
    size_t fds_sent_count = 0;
    while (fds_sent_count < txn_log_fds.size())
    {
        size_t fds_to_send_count = std::min(c_max_fd_count, txn_log_fds.size() - fds_sent_count);
        send_msg_with_fds(
            s_session_socket, txn_log_fds.data() + fds_sent_count, fds_to_send_count, msg_buf, sizeof(msg_buf));
        fds_sent_count += fds_to_send_count;
    }

    // Now we need to close all the duplicated log fds in the buffer.
    for (auto& fd : txn_log_fds)
    {
        // Each log fd should still be valid.
        ASSERT_INVARIANT(is_fd_valid(fd), "Invalid fd!");
        close_fd(fd);
    }
}

void server_t::get_txn_log_fds_for_snapshot(gaia_txn_id_t begin_ts, std::vector<int>& txn_log_fds)
{
    ASSERT_PRECONDITION(txn_log_fds.empty(), "Vector passed in to get_txn_log_fds_for_snapshot() should be empty!");

    // Take a snapshot of the post-apply watermark and scan backward from
    // begin_ts, stopping either just before the saved watermark or at the first
    // commit_ts whose log fd has been invalidated. This avoids having our scan
    // race the concurrently advancing watermark.
    gaia_txn_id_t last_known_applied_commit_ts = s_last_applied_commit_ts_lower_bound;
    for (gaia_txn_id_t ts = begin_ts - 1; ts > last_known_applied_commit_ts; --ts)
    {
        if (txn_metadata_t::is_commit_ts(ts))
        {
            ASSERT_INVARIANT(
                txn_metadata_t::is_txn_decided(ts),
                "Undecided commit_ts found in snapshot window!");
            if (txn_metadata_t::is_txn_committed(ts))
            {
                // Because the watermark could advance past its saved value, we
                // need to be sure that we don't send an invalidated and closed
                // log fd, so we validate and duplicate each log fd using the
                // safe_fd_from_ts_t class before sending it to the client. We set
                // auto_close_fd = false in the safe_fd_from_ts_t constructor
                // because we need to save the dup fds in the buffer until we
                // pass them to sendmsg().
                try
                {
                    safe_fd_from_ts_t committed_txn_log_fd(ts, false);
                    int local_log_fd = committed_txn_log_fd.get_fd();
                    txn_log_fds.push_back(local_log_fd);
                }
                catch (const invalid_log_fd&)
                {
                    // We ignore an invalidated fd because its log has already
                    // been applied to the shared locator view, so we don't need
                    // to send it to the client anyway. This means all preceding
                    // committed txns have already been applied to the shared
                    // locator view, so we can terminate the scan early.
                    break;
                }
            }
        }
    }

    // Because we scan the snapshot window backward and append fds to the buffer,
    // they are in reverse timestamp order.
    std::reverse(std::begin(txn_log_fds), std::end(txn_log_fds));
}

void server_t::handle_rollback_txn(
    int* fds, size_t fd_count, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    ASSERT_PRECONDITION(event == session_event_t::ROLLBACK_TXN, c_message_unexpected_event_received);

    // This message should only be received while a transaction is in progress.
    ASSERT_PRECONDITION(
        old_state == session_state_t::TXN_IN_PROGRESS && new_state == session_state_t::CONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    ASSERT_PRECONDITION(s_fd_log == -1, "fd log should be uninitialized!");

    // Get the log fd and free it if the client sends it.
    // The client will not send the txn log to the server if a read-only txn was rolled back.
    if (fds && fd_count == 1)
    {
        s_fd_log = *fds;
        ASSERT_INVARIANT(s_fd_log != -1, c_message_uninitialized_fd_log);
    }

    // Release all txn resources and mark the txn's begin_ts metadata as terminated.
    txn_rollback();
}

void server_t::handle_commit_txn(
    int* fds, size_t fd_count, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    ASSERT_PRECONDITION(event == session_event_t::COMMIT_TXN, c_message_unexpected_event_received);

    // This message should only be received while a transaction is in progress.
    ASSERT_PRECONDITION(
        old_state == session_state_t::TXN_IN_PROGRESS && new_state == session_state_t::TXN_COMMITTING,
        c_message_current_event_is_inconsistent_with_state_transition);

    // Get the log fd and mmap it.
    ASSERT_PRECONDITION(fds && fd_count == 1, "Invalid fd data!");
    s_fd_log = *fds;
    ASSERT_PRECONDITION(s_fd_log != -1, c_message_uninitialized_fd_log);

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
    ASSERT_PRECONDITION(seals == (F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE), "Unexpected seals on log fd!");

    // Linux won't let us create a shared read-only mapping if F_SEAL_WRITE is set,
    // which seems contrary to the manpage for fcntl(2).
    // We map the log into a local variable so it gets unmapped automatically on exit.
    mapped_log_t log;
    log.open(s_fd_log);
    // Surface the log in our static variable, to avoid passing it around.
    s_log = log.data();

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

void server_t::handle_decide_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    ASSERT_PRECONDITION(
        event == session_event_t::DECIDE_TXN_COMMIT || event == session_event_t::DECIDE_TXN_ABORT,
        c_message_unexpected_event_received);

    ASSERT_PRECONDITION(
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
    // a bug, because it provides natural backpressure on clients who submit
    // long-running transactions that prevent old versions and logs from being
    // freed. This approach helps keep the system from accumulating more
    // deferred work than it can ever retire, which is a problem with performing
    // all maintenance asynchronously in the background. Allowing this work to
    // delay beginning new transactions but not delay committing the current
    // transaction seems like a good compromise.
    perform_maintenance();
}

void server_t::handle_client_shutdown(
    int*, size_t, session_event_t event, const void*, session_state_t, session_state_t new_state)
{
    ASSERT_PRECONDITION(
        event == session_event_t::CLIENT_SHUTDOWN,
        c_message_unexpected_event_received);

    ASSERT_PRECONDITION(
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

void server_t::handle_server_shutdown(
    int*, size_t, session_event_t event, const void*, session_state_t, session_state_t new_state)
{
    ASSERT_PRECONDITION(
        event == session_event_t::SERVER_SHUTDOWN,
        c_message_unexpected_event_received);

    ASSERT_PRECONDITION(
        new_state == session_state_t::DISCONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    // This transition should only be triggered on notification of the server shutdown event.
    // Because we are about to shut down, we can't wait for acknowledgment from the client and
    // should just close the session socket. As noted above, setting the shutdown flag will
    // immediately break out of the poll loop and close the session socket.
    s_session_shutdown = true;
}

std::pair<int, int> server_t::get_stream_socket_pair()
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

    // Set server socket to be nonblocking, because we use it within an epoll loop.
    set_non_blocking(server_socket);

    socket_cleanup.dismiss();
    return std::pair{client_socket, server_socket};
}

void server_t::handle_request_stream(
    int*, size_t, session_event_t event, const void* event_data, session_state_t old_state, session_state_t new_state)
{
    ASSERT_PRECONDITION(
        event == session_event_t::REQUEST_STREAM,
        c_message_unexpected_event_received);

    // This event never changes session state.
    ASSERT_PRECONDITION(
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
    ASSERT_INVARIANT(
        request->data_type() == request_data_t::table_scan,
        c_message_unexpected_request_data_type);

    auto type = static_cast<gaia_type_t>(request->data_as_table_scan()->type_id());
    auto id_generator = get_id_generator_for_type(type);

    // We can't use structured binding names in a lambda capture list.
    int client_socket, server_socket;
    std::tie(client_socket, server_socket) = get_stream_socket_pair();

    // The client socket should unconditionally be closed on exit because it's
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

void server_t::apply_transition(session_event_t event, const void* event_data, int* fds, size_t fd_count)
{
    if (event == session_event_t::NOP)
    {
        return;
    }

    for (auto t : c_valid_transitions)
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

void server_t::build_server_reply(
    FlatBufferBuilder& builder,
    session_event_t event,
    session_state_t old_state,
    session_state_t new_state,
    gaia_txn_id_t txn_id,
    size_t log_fd_count)
{
    flatbuffers::Offset<server_reply_t> server_reply;
    const auto transaction_info = Createtransaction_info_t(builder, txn_id, log_fd_count);
    server_reply = Createserver_reply_t(
        builder, event, old_state, new_state, reply_data_t::transaction_info, transaction_info.Union());
    const auto message = Createmessage_t(builder, any_message_t::reply, server_reply.Union());
    builder.Finish(message);
}

void server_t::clear_shared_memory()
{
    s_shared_locators.close();
    s_shared_counters.close();
    s_shared_data.close();
    s_shared_id_index.close();
}

// To avoid synchronization, we assume that this method is only called when
// no sessions exist and the server is not accepting any new connections.
void server_t::init_shared_memory()
{
    // The listening socket must not be open.
    ASSERT_PRECONDITION(s_listening_socket == -1, "Listening socket should not be open!");

    // We may be reinitializing the server upon receiving a SIGHUP.
    clear_shared_memory();

    // Clear all shared memory if an exception is thrown.
    auto cleanup_memory = make_scope_guard([]() { clear_shared_memory(); });

    ASSERT_INVARIANT(!s_shared_locators.is_set(), "Locators memory should be unmapped!");
    ASSERT_INVARIANT(!s_shared_counters.is_set(), "Counters memory should be unmapped!");
    ASSERT_INVARIANT(!s_shared_data.is_set(), "Data memory should be unmapped!");
    ASSERT_INVARIANT(!s_shared_id_index.is_set(), "ID index memory should be unmapped!");

    // s_shared_locators uses sizeof(gaia_offset_t) * c_max_locators = 32GB of virtual address space.
    //
    // s_shared_data uses (64B) * c_max_locators = 256GB of virtual address space.
    //
    // s_shared_id_index uses (32B) * c_max_locators = 128GB of virtual address space
    // (assuming 4-byte alignment). We could eventually shrink this to
    // 4B/locator (assuming 4-byte locators), or 16GB, if we can assume that
    // gaia_ids are sequentially allocated and seldom deleted, so we can just
    // use an array of locators indexed by gaia_id.
    s_shared_locators.create(c_gaia_mem_locators);
    s_shared_counters.create(c_gaia_mem_counters);
    s_shared_data.create(c_gaia_mem_data);
    s_shared_id_index.create(c_gaia_mem_id_index);

    init_memory_manager();

    // Populate shared memory from the persistent log and snapshot.
    recover_db();

    cleanup_memory.dismiss();
}

void server_t::init_memory_manager()
{
    s_memory_manager.initialize(
        reinterpret_cast<uint8_t*>(s_shared_data.data()->objects),
        sizeof(s_shared_data.data()->objects));

    address_offset_t chunk_address_offset = s_memory_manager.allocate_chunk();
    if (chunk_address_offset == c_invalid_address_offset)
    {
        throw memory_allocation_error("Memory manager ran out of memory during call to allocate_chunk().");
    }
    s_chunk_manager.initialize(
        reinterpret_cast<uint8_t*>(s_shared_data.data()->objects),
        chunk_address_offset);

    auto deallocate_object_fn = [=](gaia_offset_t offset) {
        s_memory_manager.deallocate(get_address_offset(offset));
    };
    register_object_deallocator(deallocate_object_fn);
}

address_offset_t server_t::allocate_object(
    gaia_locator_t locator,
    size_t size)
{
    ASSERT_PRECONDITION(size != 0, "Size passed to server_t::allocate_object() should not be 0!");

    address_offset_t object_offset = s_chunk_manager.allocate(size + sizeof(db_object_t));
    if (object_offset == c_invalid_address_offset)
    {
        // We ran out of memory in the current chunk.
        // Mark allocations made so far as committed and allocate a new chunk!
        s_chunk_manager.commit();
        address_offset_t chunk_address_offset = s_memory_manager.allocate_chunk();
        if (chunk_address_offset == c_invalid_address_offset)
        {
            throw memory_allocation_error("Memory manager ran out of memory during call to allocate_chunk().");
        }

        s_chunk_manager.initialize(
            reinterpret_cast<uint8_t*>(s_shared_data.data()->objects),
            chunk_address_offset);

        // Allocate from new chunk.
        object_offset = s_chunk_manager.allocate(size + sizeof(db_object_t));
    }

    ASSERT_POSTCONDITION(object_offset != c_invalid_address_offset, "Chunk manager allocation was not expected to fail!");

    update_locator(locator, object_offset);

    return object_offset;
}

void server_t::recover_db()
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

sigset_t server_t::mask_signals()
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

void server_t::signal_handler(sigset_t sigset, int& signum)
{
    // Wait until a signal is delivered.
    // REVIEW: do we have any use for sigwaitinfo()?
    ::sigwait(&sigset, &signum);

    cerr << "Caught signal '" << ::strsignal(signum) << "'." << endl;

    signal_eventfd(s_server_shutdown_eventfd);
}

void server_t::init_listening_socket()
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

bool server_t::authenticate_client_socket(int socket)
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
            ASSERT_INVARIANT(iter->joinable(), c_message_thread_must_be_joinable);
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

void server_t::client_dispatch_handler()
{
    // Register session cleanup handler first, so we can execute it last.
    auto session_cleanup = make_scope_guard([]() {
        for (auto& thread : s_session_threads)
        {
            ASSERT_INVARIANT(thread.joinable(), c_message_thread_must_be_joinable);
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
            ASSERT_INVARIANT(ev.events == EPOLLIN, c_message_unexpected_event_type);

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
                ASSERT_INVARIANT(false, c_message_unexpected_fd);
            }
        }
    }
}

void server_t::session_handler(int session_socket)
{
    // Set up session socket.
    s_session_shutdown = false;
    s_session_socket = session_socket;
    auto socket_cleanup = make_scope_guard([&]() {
        // We can rely on close_fd() to perform the equivalent of
        // shutdown(SHUT_RDWR), because we hold the only fd pointing to this
        // socket. That should allow the client to read EOF if they're in a
        // blocking read and exit gracefully. (If they try to write to the
        // socket after we've closed our end, they'll receive EPIPE.) We don't
        // want to try to read any pending data from the client, because we're
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
            ASSERT_INVARIANT(thread.joinable(), c_message_thread_must_be_joinable);
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
                // NB: Because many event flags are set in combination with others, the
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
                    cerr << "Client socket error: " << ::strerror(error) << endl;
                    event = session_event_t::CLIENT_SHUTDOWN;
                }
                else if (ev.events & EPOLLHUP)
                {
                    // This flag is unmaskable, so we don't need to register for it.
                    // Both ends of the socket have issued a shutdown(SHUT_WR) or equivalent.
                    ASSERT_INVARIANT(!(ev.events & EPOLLERR), c_message_epollerr_flag_should_not_be_set);
                    event = session_event_t::CLIENT_SHUTDOWN;
                }
                else if (ev.events & EPOLLRDHUP)
                {
                    // The client has called shutdown(SHUT_WR) to signal their intention to
                    // disconnect. We do the same by closing the session socket.
                    // REVIEW: Can we get both EPOLLHUP and EPOLLRDHUP when the client half-closes
                    // the socket after we half-close it?
                    ASSERT_INVARIANT(!(ev.events & EPOLLHUP), "EPOLLHUP flag should not be set!");
                    event = session_event_t::CLIENT_SHUTDOWN;
                }
                else if (ev.events & EPOLLIN)
                {
                    ASSERT_INVARIANT(
                        !(ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)),
                        "EPOLLERR, EPOLLHUP, EPOLLRDHUP flags should not be set!");

                    // Read client message with possible file descriptors.
                    size_t bytes_read = recv_msg_with_fds(s_session_socket, fd_buf, &fd_buf_size, msg_buf, sizeof(msg_buf));
                    // We shouldn't get EOF unless EPOLLRDHUP is set.
                    // REVIEW: it might be possible for the client to call shutdown(SHUT_WR)
                    // after we have already woken up on EPOLLIN, in which case we would
                    // legitimately read 0 bytes and this assert would be invalid.
                    ASSERT_INVARIANT(bytes_read > 0, "Failed to read message!");

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
                    ASSERT_INVARIANT(false, c_message_unexpected_event_type);
                }
            }
            else if (ev.data.fd == s_server_shutdown_eventfd)
            {
                ASSERT_INVARIANT(ev.events == EPOLLIN, "Expected EPOLLIN event type!");
                consume_eventfd(s_server_shutdown_eventfd);
                event = session_event_t::SERVER_SHUTDOWN;
            }
            else
            {
                // We don't monitor any other fds.
                ASSERT_INVARIANT(false, c_message_unexpected_fd);
            }

            ASSERT_INVARIANT(event != session_event_t::NOP, c_message_unexpected_event_type);

            // The transition handlers are the only places we currently call
            // send_msg_with_fds(). We need to handle a peer_disconnected
            // exception thrown from that method (translated from EPIPE).
            try
            {
                apply_transition(event, event_data, fds, fd_count);
            }
            catch (const peer_disconnected& e)
            {
                cerr << "Client socket error: " << e.what() << endl;
                s_session_shutdown = true;
            }
        }
    }
}

template <typename T_element>
void server_t::stream_producer_handler(
    int stream_socket, int cancel_eventfd, std::function<std::optional<T_element>()> generator_fn)
{
    // We only support fixed-width integer types for now to avoid framing.
    static_assert(std::is_integral<T_element>::value, "Generator function must return an integer.");

    // The session thread gave the producer thread ownership of this socket.
    auto socket_cleanup = make_scope_guard([&]() {
        // We can rely on close_fd() to perform the equivalent of shutdown(SHUT_RDWR),
        // because we hold the only fd pointing to this socket.
        close_fd(stream_socket);
    });

    // Verify that the socket is the correct type for the semantics we assume.
    check_socket_type(stream_socket, SOCK_SEQPACKET);

    // Check that our stream socket is non-blocking (so we don't accidentally block in write()).
    ASSERT_PRECONDITION(is_non_blocking(stream_socket), "Stream socket is in blocking mode!");

    auto gen_iter = generator_iterator_t<T_element>(generator_fn);

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
    std::vector<T_element> batch_buffer;

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
                // NB: Because many event flags are set in combination with others, the
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
                    ASSERT_INVARIANT(!(ev.events & EPOLLERR), c_message_epollerr_flag_should_not_be_set);
                    producer_shutdown = true;
                }
                else if (ev.events & EPOLLOUT)
                {
                    ASSERT_INVARIANT(
                        !(ev.events & (EPOLLERR | EPOLLHUP)),
                        "EPOLLERR and EPOLLHUP flags should not be set!");

                    // Write to the send buffer until we exhaust either the iterator or the buffer free space.
                    while (gen_iter && (batch_buffer.size() < c_stream_batch_size))
                    {
                        T_element next_val = *gen_iter;
                        batch_buffer.push_back(next_val);
                        ++gen_iter;
                    }

                    // We need to send any pending data in the buffer, followed by EOF
                    // if we reached end of iteration. We let the client decide when to
                    // close the socket, because their next read may be arbitrarily delayed
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
                            stream_socket, batch_buffer.data(), batch_buffer.size() * sizeof(T_element),
                            MSG_NOSIGNAL);

                        if (bytes_written == -1)
                        {
                            // It should never happen that the socket is no longer writable
                            // after we receive EPOLLOUT, because we are the only writer and
                            // the receive buffer is always large enough for a batch.
                            ASSERT_INVARIANT(errno != EAGAIN && errno != EWOULDBLOCK, c_message_unexpected_errno_value);
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
                    // because they may still have unread data, so we don't set the
                    // producer_shutdown flag.)
                    if (!gen_iter)
                    {
                        ::shutdown(stream_socket, SHUT_WR);
                    }
                }
                else
                {
                    // We don't register for any other events.
                    ASSERT_INVARIANT(false, c_message_unexpected_event_type);
                }
            }
            else if (ev.data.fd == cancel_eventfd)
            {
                ASSERT_INVARIANT(ev.events == EPOLLIN, c_message_unexpected_event_type);
                consume_eventfd(cancel_eventfd);
                producer_shutdown = true;
            }
            else
            {
                // We don't monitor any other fds.
                ASSERT_INVARIANT(false, c_message_unexpected_fd);
            }
        }
    }
}

template <typename T_element>
void server_t::start_stream_producer(int stream_socket, std::function<std::optional<T_element>()> generator_fn)
{
    // First reap any owned threads that have terminated (to avoid memory and
    // system resource leaks).
    reap_exited_threads(s_session_owned_threads);

    // Create stream producer thread.
    s_session_owned_threads.emplace_back(
        stream_producer_handler<T_element>, stream_socket, s_session_shutdown_eventfd, generator_fn);
}

std::function<std::optional<gaia_id_t>()> server_t::get_id_generator_for_type(gaia_type_t type)
{
    record_iterator_t iterator;
    bool is_initialized = false;

    return [=]() mutable -> std::optional<gaia_id_t> {
        // The initialization of the record_list iterator should be done by the same thread
        // that executes the iteration.
        if (!is_initialized)
        {
            shared_ptr<record_list_t> record_list = record_list_manager_t::get()->get_record_list(type);
            record_list->start(iterator);
            is_initialized = true;
        }

        // Find the next valid record locator.
        while (!iterator.at_end())
        {
            gaia_locator_t locator = record_list_t::get_record_data(iterator).locator;
            ASSERT_INVARIANT(
                locator != c_invalid_gaia_locator, "An invalid locator value was returned from record list iteration!");

            db_object_t* db_object = locator_to_ptr(locator);

            record_list_t::move_next(iterator);

            if (db_object)
            {
                return db_object->id;
            }
        }

        // Signal end of scan.
        return std::nullopt;
    };
}

void server_t::validate_txns_in_range(gaia_txn_id_t start_ts, gaia_txn_id_t end_ts)
{
    // Scan txn table entries from start_ts to end_ts.
    // Seal any uninitialized entries and validate any undecided txns.
    for (gaia_txn_id_t ts = start_ts; ts < end_ts; ++ts)
    {
        // Fence off any txns that have allocated a commit_ts between start_ts
        // and end_ts but have not yet registered a commit_ts metadata in the txn
        // table.
        if (txn_metadata_t::seal_uninitialized_ts(ts))
        {
            continue;
        }

        // Validate any undecided submitted txns.
        if (txn_metadata_t::is_commit_ts(ts) && txn_metadata_t::is_txn_validating(ts))
        {
            bool is_committed = validate_txn(ts);

            // Update the current txn's decided status.
            txn_metadata_t::update_txn_decision(ts, is_committed);
        }
    }
}

gaia_txn_id_t server_t::submit_txn(gaia_txn_id_t begin_ts, int log_fd)
{
    ASSERT_PRECONDITION(txn_metadata_t::is_txn_active(begin_ts), "Not an active transaction!");

    // We assume all our log fds are non-negative and fit into 16 bits. (The
    // highest possible value is reserved to indicate an invalid fd.)
    ASSERT_PRECONDITION(
        log_fd >= 0 && log_fd < std::numeric_limits<uint16_t>::max(),
        "Transaction log fd is not between 0 and (2^16 - 2)!");

    ASSERT_PRECONDITION(is_fd_valid(log_fd), "Invalid log fd!");

    // Allocate a new commit_ts and initialize its metadata with our begin_ts and log fd.
    gaia_txn_id_t commit_ts = txn_metadata_t::register_commit_ts(begin_ts, log_fd);

    // Now update the active txn metadata.
    txn_metadata_t::set_active_txn_submitted(begin_ts, commit_ts);

    return commit_ts;
}

// This helper method takes 2 file descriptors for txn logs and determines if
// they have a non-empty intersection. We could just use std::set_intersection,
// but that outputs all elements of the intersection in a third container, while
// we just need to test for non-empty intersection (and terminate as soon as the
// first common element is found), so we write our own simple merge intersection
// code. If we ever need to return the IDs of all conflicting objects to clients
// (or just log them for diagnostics), we could use std::set_intersection.
bool server_t::txn_logs_conflict(int log_fd1, int log_fd2)
{
    // First map the two fds.
    mapped_log_t log1;
    log1.open(log_fd1);
    mapped_log_t log2;
    log2.open(log_fd2);

    // Now perform standard merge intersection and terminate on the first conflict found.
    size_t log1_idx = 0, log2_idx = 0;
    while (log1_idx < log1.data()->record_count && log2_idx < log2.data()->record_count)
    {
        txn_log_t::log_record_t* lr1 = log1.data()->log_records + log1_idx;
        txn_log_t::log_record_t* lr2 = log2.data()->log_records + log2_idx;

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

bool server_t::validate_txn(gaia_txn_id_t commit_ts)
{
    // Validation algorithm:

    // NB: We make two passes over the conflict window, even though only one
    // pass would suffice, because we want to avoid unnecessary validation of
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
    //    within our conflict window to be registered. Seal all uninitialized
    //    timestamps we encounter, so no active txns can be submitted within our
    //    conflict window. Any submitted txns which have allocated a commit
    //    timestamp within our conflict window but have not yet registered their
    //    commit timestamp must now retry with a new timestamp. (This allows us
    //    to treat our conflict window as an immutable snapshot of submitted
    //    txns, without needing to acquire any locks.) Skip over all sealed
    //    timestamps, active txns, undecided txns, and aborted txns. Repeat this
    //    phase until no newly committed txns are discovered.
    //
    // 2. Recursively validate all undecided txns in the conflict window, from
    //    oldest to newest. Note that we cannot recurse indefinitely, because by
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
    //    In either case, we set the decided state in our commit timestamp metadata
    //    and return the commit decision to the client.
    //
    // REVIEW: Possible optimization for conflict detection, because mmap() is so
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
    // need to be validated, because they could conflict with a later undecided
    // txn with a conflicting write set, which would abort and hence not cause
    // us to abort).

    // Because we make multiple passes over the conflict window, we need to track
    // committed txns that have already been tested for conflicts.
    std::unordered_set<gaia_txn_id_t> committed_txns_tested_for_conflicts;

    // Iterate over all txns in conflict window, and test all committed txns for
    // conflicts. Repeat until no new committed txns are discovered. This gives
    // undecided txns as much time as possible to be validated by their
    // committing txn.
    bool has_found_new_committed_txn;
    do
    {
        has_found_new_committed_txn = false;
        for (gaia_txn_id_t ts = txn_metadata_t::get_begin_ts(commit_ts) + 1; ts < commit_ts; ++ts)
        {
            // Seal all uninitialized timestamps. This marks a "fence" after which
            // any submitted txns with commit timestamps in our conflict window must
            // already be registered under their commit_ts (they must retry with a
            // new timestamp otherwise). (The sealing is necessary only on the
            // first pass, but the "uninitialized txn metadata" check is cheap enough
            // that repeating it on subsequent passes shouldn't matter.)
            if (txn_metadata_t::seal_uninitialized_ts(ts))
            {
                continue;
            }

            if (txn_metadata_t::is_commit_ts(ts) && txn_metadata_t::is_txn_committed(ts))
            {
                // Remember each committed txn commit_ts so we don't test it again.
                const auto [iter, is_new_committed_ts] = committed_txns_tested_for_conflicts.insert(ts);
                if (is_new_committed_ts)
                {
                    has_found_new_committed_txn = true;

                    // Eagerly test committed txns for conflicts to give undecided
                    // txns more time for validation (and submitted txns more time
                    // for registration).

                    // We need to use the safe_fd_from_ts_t wrapper for conflict detection
                    // in case either log fd is invalidated by another thread
                    // concurrently advancing the watermark. If either log fd is
                    // invalidated, it must be that another thread has validated our
                    // txn, so we can exit early.
                    try
                    {
                        safe_fd_from_ts_t validating_fd(commit_ts);
                        safe_fd_from_ts_t committed_fd(ts);

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
                        // txn has already been validated. Because our commit_ts always
                        // follows the commit_ts of the undecided txn we are testing for
                        // conflicts, and the watermark always advances in order, it
                        // cannot be the case that this txn's log fd has not been
                        // invalidated while our txn's log fd has been invalidated.
                        gaia_txn_id_t sealed_commit_ts = e.get_ts();
                        ASSERT_INVARIANT(
                            txn_metadata_t::is_txn_decided(sealed_commit_ts),
                            c_message_txn_log_fd_cannot_be_invalidated);

                        if (sealed_commit_ts == ts)
                        {
                            ASSERT_INVARIANT(
                                txn_metadata_t::is_txn_decided(commit_ts),
                                c_message_validating_txn_should_have_been_validated);
                        }

                        // If either log fd was invalidated, then the validating txn
                        // must have been validated, so we can return the decision
                        // immediately.
                        return txn_metadata_t::is_txn_committed(commit_ts);
                    }
                }
            }

            // Check if another thread has already validated this txn.
            if (txn_metadata_t::is_txn_decided(commit_ts))
            {
                return txn_metadata_t::is_txn_committed(commit_ts);
            }
        }
    } while (has_found_new_committed_txn);

    // Validate all undecided txns, from oldest to newest. If any validated txn
    // commits, test it immediately for conflicts. Also test any committed txns
    // for conflicts if they weren't tested in the first pass.
    for (gaia_txn_id_t ts = txn_metadata_t::get_begin_ts(commit_ts) + 1; ts < commit_ts; ++ts)
    {
        if (txn_metadata_t::is_commit_ts(ts))
        {
            // Validate any currently undecided txn.
            if (txn_metadata_t::is_txn_validating(ts))
            {
                // By hypothesis, there are no undecided txns with commit timestamps
                // preceding the committing txn's begin timestamp.
                ASSERT_INVARIANT(
                    ts > s_txn_id,
                    c_message_preceding_txn_should_have_been_validated);

                // Recursively validate the current undecided txn.
                bool is_committed = validate_txn(ts);

                // Update the current txn's decided status.
                txn_metadata_t::update_txn_decision(ts, is_committed);
            }

            // If a previously undecided txn has now committed, test it for conflicts.
            if (txn_metadata_t::is_txn_committed(ts) && committed_txns_tested_for_conflicts.count(ts) == 0)
            {
                // We need to use the safe_fd_from_ts_t wrapper for conflict
                // detection in case either log fd is invalidated by another
                // thread concurrently advancing the watermark. If either log fd
                // is invalidated, it must be that another thread has validated
                // our txn, so we can exit early.
                try
                {
                    safe_fd_from_ts_t validating_fd(commit_ts);
                    safe_fd_from_ts_t new_committed_fd(ts);

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
                    // Because our commit_ts always follows the commit_ts of the
                    // undecided txn we are testing for conflicts, and the
                    // watermark always advances in order, it cannot be the case
                    // that this txn's log fd has not been invalidated while our
                    // txn's log fd has been invalidated.
                    gaia_txn_id_t sealed_commit_ts = e.get_ts();
                    ASSERT_INVARIANT(
                        txn_metadata_t::is_txn_decided(sealed_commit_ts),
                        c_message_txn_log_fd_cannot_be_invalidated);

                    if (sealed_commit_ts == commit_ts)
                    {
                        ASSERT_INVARIANT(
                            txn_metadata_t::get_txn_log_fd(ts) == -1,
                            c_message_txn_log_fd_should_have_been_invalidated);
                    }
                    else
                    {
                        ASSERT_INVARIANT(
                            sealed_commit_ts == ts,
                            c_message_unexpected_commit_ts_value);
                        ASSERT_INVARIANT(
                            txn_metadata_t::is_txn_decided(commit_ts),
                            c_message_validating_txn_should_have_been_validated);
                    }

                    // If either log fd was invalidated, then the validating txn
                    // must have been validated, so we can return the decision
                    // immediately.
                    return txn_metadata_t::is_txn_committed(commit_ts);
                }
            }
        }

        // Check if another thread has already validated this txn.
        if (txn_metadata_t::is_txn_decided(commit_ts))
        {
            return txn_metadata_t::is_txn_committed(commit_ts);
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
// NB: we use compare_exchange_weak() for the global update because we need to
// retry anyway on concurrent updates, so tolerating spurious failures
// requires no additional logic.
bool server_t::advance_watermark(std::atomic<gaia_txn_id_t>& watermark, gaia_txn_id_t ts)
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

void server_t::apply_txn_log_from_ts(gaia_txn_id_t commit_ts)
{
    ASSERT_PRECONDITION(
        txn_metadata_t::is_commit_ts(commit_ts) && txn_metadata_t::is_txn_committed(commit_ts),
        "apply_txn_log_from_ts() must be called on the commit_ts of a committed txn!");

    // Because txn logs are only eligible for GC after they fall behind the
    // post-apply watermark, we don't need the safe_fd_from_ts_t wrapper.
    int log_fd = txn_metadata_t::get_txn_log_fd(commit_ts);

    // A txn log fd should never be invalidated until it falls behind the
    // post-apply watermark.
    ASSERT_INVARIANT(
        log_fd != -1,
        "apply_txn_log_from_ts() must be called on a commit_ts with a valid log fd!");

    mapped_log_t txn_log;
    txn_log.open(log_fd);

    // Ensure that the begin_ts in this metadata matches the txn log header.
    ASSERT_INVARIANT(
        txn_log.data()->begin_ts == txn_metadata_t::get_begin_ts(commit_ts),
        "txn log begin_ts must match begin_ts reference in commit_ts metadata!");

    for (size_t i = 0; i < txn_log.data()->record_count; ++i)
    {
        // Update the shared locator view with each redo version (i.e., the
        // version created or updated by the txn). This is safe as long as the
        // committed txn being applied has commit_ts older than the oldest
        // active txn's begin_ts (so it can't overwrite any versions visible in
        // that txn's snapshot). This update is non-atomic because log application
        // is idempotent and therefore a txn log can be re-applied over the same
        // txn's partially-applied log during snapshot reconstruction.
        txn_log_t::log_record_t* lr = &(txn_log.data()->log_records[i]);
        (*s_shared_locators.data())[lr->locator] = lr->new_offset;

        // In case of insertions, we want to update the record list for the object's type.
        // We do this after updating the shared locator view, so we can access the new object's data.
        if (lr->old_offset == c_invalid_gaia_offset)
        {
            gaia_locator_t locator = lr->locator;
            db_object_t* db_object = locator_to_ptr(locator);
            std::shared_ptr<record_list_t> record_list = record_list_manager_t::get()->get_record_list(db_object->type);
            record_list->add(locator);
        }
    }

    // We're using the otherwise-unused first entry of the "locators" array to
    // track the last-applied commit_ts (purely for diagnostic purposes).
    bool has_updated_locators_view_version = advance_watermark((*s_shared_locators.data())[0], commit_ts);
    ASSERT_POSTCONDITION(
        has_updated_locators_view_version,
        "Committed txn applied to shared locators view out of order!");
}

void server_t::gc_txn_log_from_fd(int log_fd, bool committed)
{
    mapped_log_t txn_log;
    txn_log.open(log_fd);

    ASSERT_INVARIANT(txn_log.is_set(), "txn_log should be mapped when deallocating old offsets.");

    bool deallocate_new_offsets = !committed;
    deallocate_txn_log(txn_log.data(), deallocate_new_offsets);
}

void server_t::deallocate_txn_log(txn_log_t* txn_log, bool deallocate_new_offsets)
{
    ASSERT_PRECONDITION(txn_log, "txn_log must be a valid mapped address!");

    for (size_t i = 0; i < txn_log->record_count; ++i)
    {
        // For committed txns, free each undo version (i.e., the version
        // superseded by an update or delete operation), using the registered
        // object deallocator (if it exists), because the redo versions may still
        // be visible but the undo versions cannot be. For aborted or
        // rolled-back txns, free only the redo versions (because the undo
        // versions may still be visible).
        // NB: we can't safely free the redo versions and txn logs of aborted
        // txns in the decide handler, because concurrently validating txns
        // might be testing the aborted txn for conflicts while they still think
        // it is undecided. The only safe place to free the redo versions and
        // txn log of an aborted txn is after it falls behind the watermark,
        // because at that point it cannot be in the conflict window of any
        // committing txn.
        gaia_offset_t offset_to_free = deallocate_new_offsets
            ? txn_log->log_records[i].new_offset
            : txn_log->log_records[i].old_offset;

        if (offset_to_free && s_object_deallocator_fn)
        {
            // TODO: If a chunk gets freed as a result of this deallocation,
            // we should mark the chunk as freed as well.
            // Also, depending on whether we rollback the memory allocations
            // there may be no need to make deallocations here for new offsets;
            // we would only need to do deallocations for old offsets in commit scenarios.
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
//    and if the next txn metadata is either a sealed timestamp (we
//    seal all uninitialized entries during the scan), a commit_ts that is
//    decided, or a begin_ts that is terminated or submitted with a decided
//    commit_ts. If we successfully advanced the watermark and the current entry
//    is a committed commit_ts, then we can apply its redo log to the shared
//    view. After applying it (or immediately after advancing the pre-apply
//    watermark if the current timestamp is not a committed commit_ts), we
//    advance the post-apply watermark to the same timestamp. (Because we "own"
//    the current txn metadata after a successful CAS on the pre-apply
//    watermark, we can advance the post-apply watermark without a CAS.) Because
//    the pre-apply watermark can only move forward, updates to the shared view
//    are applied in timestamp order, and because the pre-apply watermark can only
//    be advanced if the post-apply watermark has caught up with it (which can
//    only be the case for a committed commit_ts if the redo log has been fully
//    applied), updates to the shared view are never applied concurrently.
//
// 2. We scan the interval from a snapshot of the post-GC watermark to a
//    snapshot of the post-apply watermark. If the current timestamp is not a
//    commit_ts, we continue the scan. Otherwise we check if its log fd is
//    invalidated. If so, then we know that GC is in progress or complete, so we
//    continue the scan. If persistence is enabled then we also check the
//    durable flag on the current txn metadata and abort the scan if it is
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
// calculated from a non-atomic scan, because any thread's observed value can only
// go forward). This means that a thread must register its observed value in the
// global table every time it reads the post-GC watermark, and should clear its
// entry when it's done with the observed value. (A subtler issue is that
// reading the current watermark value and registering the read value in the
// global table must be an atomic operation for the non-atomic minimum scan to
// be correct, but no CPU has an atomic memory-to-memory copy operation. We can
// work around this by just checking the current value of the watermark
// immediately after writing its saved value to the global table. If they're the
// same, then no other thread could have observed any non-atomicity in the copy
// operation. If they differ, then we just repeat the sequence. Because this is a
// very fast sequence of operations, retries should be infrequent, so livelock
// shouldn't be an issue.)
void server_t::perform_maintenance()
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

void server_t::apply_txn_logs_to_shared_view()
{
    // First get a snapshot of the timestamp counter for an upper bound on
    // the scan (we don't know yet if this is a begin_ts or commit_ts).
    gaia_txn_id_t last_allocated_ts = get_last_txn_id();

    // Now get a snapshot of the last_applied_commit_ts_upper_bound watermark,
    // for a lower bound on the scan.
    gaia_txn_id_t last_applied_commit_ts_upper_bound = s_last_applied_commit_ts_upper_bound;

    // Scan from the saved pre-apply watermark to the last known timestamp,
    // and apply all committed txn logs from the longest prefix of decided
    // txns that does not overlap with the conflict window of any undecided
    // txn. Advance the pre-apply watermark before applying the txn log
    // of a committed txn, and advance the post-apply watermark after
    // applying the txn log.
    for (gaia_txn_id_t ts = last_applied_commit_ts_upper_bound + 1; ts <= last_allocated_ts; ++ts)
    {
        // We need to seal uninitialized entries as we go along, so that we
        // don't miss any active begin_ts or committed commit_ts entries.
        //
        // We continue processing sealed timestamps
        // so that we can advance the pre-apply watermark over them.
        txn_metadata_t::seal_uninitialized_ts(ts);

        // If this is a commit_ts, we cannot advance the watermark unless it's
        // decided.
        if (txn_metadata_t::is_commit_ts(ts) && txn_metadata_t::is_txn_validating(ts))
        {
            break;
        }

        // The watermark cannot be advanced past any begin_ts whose txn is not
        // either in the TXN_TERMINATED state or in the TXN_SUBMITTED state with
        // its commit_ts in the TXN_DECIDED state. This means that the watermark
        // can never advance into the conflict window of an undecided txn.
        if (txn_metadata_t::is_begin_ts(ts))
        {
            if (txn_metadata_t::is_txn_active(ts))
            {
                break;
            }

            if (txn_metadata_t::is_txn_submitted(ts)
                && txn_metadata_t::is_txn_validating(txn_metadata_t::get_commit_ts(ts)))
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
        // txn metadata, but allows us to avoid reserving another scarce bit
        // for this purpose.

        // The current timestamp in the scan is guaranteed to be positive due to
        // the loop precondition.
        gaia_txn_id_t prev_ts = ts - 1;

        // This thread must have observed both the pre- and post-apply
        // watermarks to be equal to the previous timestamp in the scan in order
        // to advance the pre-apply watermark to the current timestamp in the
        // scan. This means that any thread applying a txn log at the previous
        // timestamp must have finished applying the log, so we can safely apply
        // the log at the current timestamp.
        //
        // REVIEW: These loads could be relaxed, because a stale read could only
        // result in premature abort of the scan.
        if (s_last_applied_commit_ts_upper_bound != prev_ts || s_last_applied_commit_ts_lower_bound != prev_ts)
        {
            break;
        }

        if (!advance_watermark(s_last_applied_commit_ts_upper_bound, ts))
        {
            // If another thread has already advanced the watermark ahead of
            // this ts, we abort advancing it further.
            ASSERT_INVARIANT(
                s_last_applied_commit_ts_upper_bound > last_applied_commit_ts_upper_bound,
                "The watermark must have advanced if advance_watermark() failed!");

            break;
        }

        if (txn_metadata_t::is_commit_ts(ts))
        {
            ASSERT_INVARIANT(
                txn_metadata_t::is_txn_decided(ts),
                "The watermark should not be advanced to an undecided commit_ts!");

            if (txn_metadata_t::is_txn_committed(ts))
            {
                // If a new txn starts after or while we apply this txn log to
                // the shared view, but before we advance the post-apply
                // watermark, it will re-apply some of our updates to its
                // snapshot of the shared view, but that is benign because log
                // replay is idempotent (as long as logs are applied in
                // timestamp order).
                apply_txn_log_from_ts(ts);
            }
        }

        // Now we advance the post-apply watermark to catch up with the pre-apply watermark.
        // REVIEW: Because no other thread can concurrently advance the post-apply watermark,
        // we don't need a full CAS here.
        bool has_advanced_watermark = advance_watermark(s_last_applied_commit_ts_lower_bound, ts);

        // No other thread should be able to advance the post-apply watermark,
        // because only one thread can advance the pre-apply watermark to this
        // timestamp.
        ASSERT_INVARIANT(has_advanced_watermark, "Couldn't advance the post-apply watermark!");
    }
}

void server_t::gc_applied_txn_logs()
{
    // First get a snapshot of the post-apply watermark, for an upper bound on the scan.
    gaia_txn_id_t last_applied_commit_ts_lower_bound = s_last_applied_commit_ts_lower_bound;

    // Now get a snapshot of the post-GC watermark, for a lower bound on the scan.
    gaia_txn_id_t last_freed_commit_ts_lower_bound = s_last_freed_commit_ts_lower_bound;

    // Scan from the post-GC watermark to the post-apply watermark,
    // executing GC on any commit_ts if the log fd is valid (and the durable
    // flag is set if persistence is enabled). (If we fail to invalidate the log
    // fd, we abort the scan to avoid contention.) When GC is complete, set the
    // TXN_GC_COMPLETE flag on the txn metadata and continue.
    for (gaia_txn_id_t ts = last_freed_commit_ts_lower_bound + 1; ts <= last_applied_commit_ts_lower_bound; ++ts)
    {
        ASSERT_INVARIANT(
            !txn_metadata_t::is_uninitialized_ts(ts),
            "All uninitialized txn table entries should be sealed!");

        ASSERT_INVARIANT(
            !(txn_metadata_t::is_begin_ts(ts) && txn_metadata_t::is_txn_active(ts)),
            "The watermark should not be advanced to an active begin_ts!");

        if (txn_metadata_t::is_commit_ts(ts))
        {
            // If persistence is enabled, then we also need to check that
            // TXN_PERSISTENCE_COMPLETE is set (to avoid having redo versions
            // deallocated while they're being persisted).
            bool is_persistence_enabled = (s_persistence_mode != persistence_mode_t::e_disabled)
                && (s_persistence_mode != persistence_mode_t::e_disabled_after_recovery);

            if (is_persistence_enabled && !txn_metadata_t::is_txn_durable(ts))
            {
                break;
            }

            int log_fd = txn_metadata_t::get_txn_log_fd(ts);

            // Continue the scan if this log fd has already been invalidated,
            // because GC has already been started.
            if (log_fd == -1)
            {
                continue;
            }

            // Invalidate the log fd, GC the obsolete versions in the log, and close the fd.
            // Abort the scan when invalidation fails, to avoid contention.
            if (!txn_metadata_t::invalidate_txn_log_fd(ts))
            {
                break;
            }

            // The log fd can't be closed until after it has been invalidated.
            ASSERT_INVARIANT(is_fd_valid(log_fd), "The log fd cannot be closed if we successfully invalidated it!");

            // Because we invalidated the log fd, we now hold the only accessible
            // copy of the fd, so we need to ensure it is closed.
            auto cleanup_fd = make_scope_guard([&]() { close_fd(log_fd); });

            // If the txn committed, we deallocate only undo versions, because the
            // redo versions may still be visible after the txn has fallen
            // behind the watermark. If the txn aborted, then we deallocate only
            // redo versions, because the undo versions may still be visible. Note
            // that we could deallocate intermediate versions (i.e., those
            // superseded within the same txn) immediately, but we do it here
            // for simplicity.
            gc_txn_log_from_fd(log_fd, txn_metadata_t::is_txn_committed(ts));

            // We need to mark this txn metadata TXN_GC_COMPLETE to allow the
            // post-GC watermark to advance.
            bool has_set_metadata = txn_metadata_t::set_txn_gc_complete(ts);

            // If persistence is enabled, then this commit_ts must have been
            // marked durable before we advanced the watermark, and no other
            // thread can set TXN_GC_COMPLETE after we invalidate the log fd, so
            // it should not be possible for this CAS to fail.
            ASSERT_INVARIANT(has_set_metadata, "This txn metadata cannot change after we invalidate the log fd!");
        }
    }
}

void server_t::update_txn_table_safe_truncation_point()
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
        ASSERT_INVARIANT(
            !txn_metadata_t::is_uninitialized_ts(ts),
            "All uninitialized txn table entries should be sealed!");

        ASSERT_INVARIANT(
            !(txn_metadata_t::is_begin_ts(ts) && txn_metadata_t::is_txn_active(ts)),
            "The watermark should not be advanced to an active begin_ts!");

        if (txn_metadata_t::is_commit_ts(ts))
        {
            ASSERT_INVARIANT(
                txn_metadata_t::is_txn_decided(ts),
                "The watermark should not be advanced to an undecided commit_ts!");

            // We can only advance the post-GC watermark to a commit_ts if it is
            // marked TXN_GC_COMPLETE.
            if (!txn_metadata_t::is_txn_gc_complete(ts))
            {
                break;
            }
        }

        if (!advance_watermark(s_last_freed_commit_ts_lower_bound, ts))
        {
            // If another thread has already advanced the post-GC watermark
            // ahead of this ts, we abort advancing it further.
            ASSERT_INVARIANT(
                s_last_freed_commit_ts_lower_bound > last_freed_commit_ts_lower_bound,
                "The watermark must have advanced if advance_watermark() failed!");

            break;
        }

        // There are no actions to take after advancing the post-GC watermark,
        // because the post-GC watermark only exists to provide a safe lower
        // bound for truncating transaction history.
    }
}

void server_t::txn_rollback()
{
    ASSERT_PRECONDITION(
        s_txn_id != c_invalid_gaia_txn_id,
        "txn_rollback() was called without an active transaction!");

    auto cleanup_log_fd = make_scope_guard([&]() {
        close_fd(s_fd_log);
    });

    // Set our txn status to TXN_TERMINATED.
    // NB: this must be done before calling perform_maintenance()!
    txn_metadata_t::set_active_txn_terminated(s_txn_id);

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
bool server_t::txn_commit()
{
    ASSERT_PRECONDITION(s_fd_log != -1, c_message_uninitialized_fd_log);

    // Register the committing txn under a new commit timestamp.
    gaia_txn_id_t commit_ts = submit_txn(s_txn_id, s_fd_log);

    // This is only used for persistence.
    std::string txn_name;

    if (rdb)
    {
        txn_name = rdb->begin_txn(s_txn_id);
        // Prepare log for transaction.
        // This is effectively asynchronous with validation, because if it takes
        // too long, then another thread may recursively validate this txn,
        // before the committing thread has a chance to do so.
        rdb->prepare_wal_for_write(s_log, txn_name);
    }

    ASSERT_INVARIANT(s_fd_log != -1, c_message_uninitialized_fd_log);

    // Validate the txn against all other committed txns in the conflict window.
    bool is_committed = validate_txn(commit_ts);

    // Update the txn metadata with our commit decision.
    txn_metadata_t::update_txn_decision(commit_ts, is_committed);

    // Persist the commit decision.
    // REVIEW: We can return a decision to the client asynchronously with the
    // decision being persisted (because the decision can be reconstructed from
    // the durable log itself, without the decision record).
    if (rdb)
    {
        // Mark txn as durable in metadata so we can GC the txn log.
        // We only mark it durable after validation to simplify the
        // state transitions:
        // TXN_VALIDATING -> TXN_DECIDED -> TXN_DURABLE.
        txn_metadata_t::set_txn_durable(commit_ts);

        if (is_committed)
        {
            rdb->append_wal_commit_marker(txn_name);
        }
        else
        {
            rdb->append_wal_rollback_marker(txn_name);
        }
    }

    return is_committed;
}

// this must be run on main thread
// see https://thomastrapp.com/blog/signal-handler-for-multithreaded-c++/
void server_t::run(persistence_mode_t persistence_mode)
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
        txn_metadata_t::init_txn_metadata_map();

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
        ASSERT_INVARIANT(caught_signal != 0, "A signal should have been caught!");

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
            // That is benign, because we've already performed cleanup actions
            // and the exit status will still be valid.
            ::pthread_sigmask(SIG_UNBLOCK, &handled_signals, nullptr);
            ::raise(caught_signal);
        }
    }
}
