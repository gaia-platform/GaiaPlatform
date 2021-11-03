/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_server.hpp"

#include <unistd.h>

#include <csignal>

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <thread>
#include <unordered_set>

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "gaia/exceptions.hpp"

#include "gaia_internal/common/memory_allocation_error.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/common/socket_helpers.hpp"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"
#include "gaia_internal/db/index_builder.hpp"

#include "gaia_spdlog/fmt/fmt.h"

#include "bitmap.hpp"
#include "db_helpers.hpp"
#include "db_internal_types.hpp"
#include "memory_helpers.hpp"
#include "memory_types.hpp"
#include "messages_generated.h"
#include "persistent_store_manager.hpp"
#include "record_list_manager.hpp"
#include "system_checks.hpp"
#include "txn_metadata.hpp"
#include "type_generator.hpp"
#include "type_id_mapping.hpp"

using namespace flatbuffers;
using namespace gaia::db;
using namespace gaia::db::messages;
using namespace gaia::db::memory_manager;
using namespace gaia::db::storage;
using namespace gaia::db::transactions;
using namespace gaia::common;
using namespace gaia::common::iterators;
using namespace gaia::common::scope_guard;

using persistence_mode_t = server_config_t::persistence_mode_t;

static constexpr char c_message_uninitialized_log_fd[] = "Uninitialized log fd!";
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
static constexpr char c_message_unexpected_query_type[] = "Unexpected query type!";

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

    // Collect fds.
    int fd_list[static_cast<size_t>(data_mapping_t::index_t::count_mappings)];
    data_mapping_t::collect_fds(c_data_mappings, fd_list);

    send_msg_with_fds(
        s_session_socket,
        &fd_list[0],
        static_cast<size_t>(data_mapping_t::index_t::count_mappings),
        builder.GetBufferPointer(),
        builder.GetSize());
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

    ASSERT_PRECONDITION(!s_log.is_set(), "Transaction log should be uninitialized!");

    // REVIEW: we could make this a session thread-local to avoid dynamic
    // allocation per txn.
    std::vector<int> txn_log_fds_for_snapshot;
    auto cleanup_txn_log_fds_for_snapshot = make_scope_guard([&]() {
        // Close all the duplicated log fds in the buffer.
        for (int& fd : txn_log_fds_for_snapshot)
        {
            // Each log fd should still be valid.
            ASSERT_INVARIANT(is_fd_valid(fd), "Invalid fd!");
            close_fd(fd);
        }
    });
    txn_begin(txn_log_fds_for_snapshot);

    // Send the reply message to the client, with the number of txn log fds to
    // be sent later.
    // REVIEW: We could optimize the fast path by piggybacking some small fixed
    // number of log fds on the initial reply message.
    int log_fd = s_log.fd();
    FlatBufferBuilder builder;
    build_server_reply(builder, session_event_t::BEGIN_TXN, old_state, new_state, s_txn_id, txn_log_fds_for_snapshot.size());
    send_msg_with_fds(s_session_socket, &log_fd, 1, builder.GetBufferPointer(), builder.GetSize());

    // Send all txn log fds to the client in an additional sequence of dummy messages.
    // We need a 1-byte dummy message buffer due to our datagram size convention.
    uint8_t msg_buf[1]{0};
    size_t fds_sent_count = 0;
    while (fds_sent_count < txn_log_fds_for_snapshot.size())
    {
        size_t fds_to_send_count = std::min(c_max_fd_count, txn_log_fds_for_snapshot.size() - fds_sent_count);
        send_msg_with_fds(
            s_session_socket, txn_log_fds_for_snapshot.data() + fds_sent_count, fds_to_send_count, msg_buf, sizeof(msg_buf));
        fds_sent_count += fds_to_send_count;
    }
}

void server_t::txn_begin(std::vector<int>& txn_log_fds_for_snapshot)
{
    // Allocate a new begin_ts for this txn and initialize its metadata in the txn table.
    s_txn_id = txn_metadata_t::register_begin_ts();

    // The begin_ts returned by register_begin_ts() should always be valid because it
    // retries on concurrent invalidation.
    ASSERT_INVARIANT(s_txn_id != c_invalid_gaia_txn_id, "Begin timestamp is invalid!");

    // Ensure that there are no undecided txns in our snapshot window.
    gaia_txn_id_t pre_apply_watermark = get_watermark(watermark_type_t::pre_apply);
    validate_txns_in_range(pre_apply_watermark + 1, s_txn_id);

    get_txn_log_fds_for_snapshot(s_txn_id, txn_log_fds_for_snapshot);

    // Allocate the txn log fd on the server, for rollback-safety if the client session crashes.
    s_log.create(gaia_fmt::format("{}{}:{}", c_gaia_internal_txn_log_prefix, s_server_conf.instance_name(), s_txn_id).c_str());

    // Update the log header with our begin timestamp and initialize it to empty.
    s_log.data()->begin_ts = s_txn_id;
    s_log.data()->current_chunk = c_invalid_chunk_offset;
    s_log.data()->record_count = 0;
}

void server_t::get_txn_log_fds_for_snapshot(gaia_txn_id_t begin_ts, std::vector<int>& txn_log_fds)
{
    ASSERT_PRECONDITION(txn_log_fds.empty(), "Vector passed in to get_txn_log_fds_for_snapshot() should be empty!");

    // Take a snapshot of the post-apply watermark and scan backward from
    // begin_ts, stopping either just before the saved watermark or at the first
    // commit_ts whose log fd has been invalidated. This avoids having our scan
    // race the concurrently advancing watermark.
    gaia_txn_id_t post_apply_watermark = get_watermark(watermark_type_t::post_apply);
    for (gaia_txn_id_t ts = begin_ts - 1; ts > post_apply_watermark; --ts)
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
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    ASSERT_PRECONDITION(event == session_event_t::ROLLBACK_TXN, c_message_unexpected_event_received);

    // This message should only be received while a transaction is in progress.
    ASSERT_PRECONDITION(
        old_state == session_state_t::TXN_IN_PROGRESS && new_state == session_state_t::CONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    ASSERT_PRECONDITION(
        s_txn_id != c_invalid_gaia_txn_id,
        "No active transaction to roll back!");

    ASSERT_PRECONDITION(s_log.is_set(), c_message_uninitialized_log_fd);

    // Release all txn resources and mark the txn's begin_ts metadata as terminated.
    txn_rollback();
}

void server_t::handle_commit_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    ASSERT_PRECONDITION(event == session_event_t::COMMIT_TXN, c_message_unexpected_event_received);

    // This message should only be received while a transaction is in progress.
    ASSERT_PRECONDITION(
        old_state == session_state_t::TXN_IN_PROGRESS && new_state == session_state_t::TXN_COMMITTING,
        c_message_current_event_is_inconsistent_with_state_transition);

    ASSERT_PRECONDITION(s_log.is_set(), c_message_uninitialized_log_fd);

    // Actually commit the transaction.
    session_event_t decision = session_event_t::NOP;
    try
    {
        bool success = txn_commit();
        decision = success ? session_event_t::DECIDE_TXN_COMMIT : session_event_t::DECIDE_TXN_ABORT;
    }
    catch (const index::unique_constraint_violation& e)
    {
        // Rollback our transaction in case of constraint violations.
        // This is because such failures happen in the early pre-commit phase.
        txn_rollback();

        decision = session_event_t::DECIDE_TXN_ROLLBACK_UNIQUE;
    }

    // Server-initiated state transition! (Any issues with reentrant handlers?)
    apply_transition(decision, nullptr, nullptr, 0);
}

void server_t::handle_decide_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    ASSERT_PRECONDITION(
        event == session_event_t::DECIDE_TXN_COMMIT
            || event == session_event_t::DECIDE_TXN_ABORT
            || event == session_event_t::DECIDE_TXN_ROLLBACK_UNIQUE,
        c_message_unexpected_event_received);

    ASSERT_PRECONDITION(
        old_state == session_state_t::TXN_COMMITTING && new_state == session_state_t::CONNECTED,
        c_message_current_event_is_inconsistent_with_state_transition);

    // We need to clear transactional state after the decision has been
    // returned, but we don't close the log fd (even for an abort decision),
    // because GC needs the log in order to properly deallocate all allocations
    // made by this txn when they become obsolete.
    auto cleanup = make_scope_guard([&]() { s_txn_id = c_invalid_gaia_txn_id; });

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
        bool client_disconnected = true;
        txn_rollback(client_disconnected);
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

    // We can't use structured binding names in a lambda capture list.
    int client_socket, server_socket;
    std::tie(client_socket, server_socket) = get_stream_socket_pair();

    // The client socket should unconditionally be closed on exit because it's
    // duplicated when passed to the client and we no longer need it on the
    // server.
    auto client_socket_cleanup = make_scope_guard([&]() { close_fd(client_socket); });
    auto server_socket_cleanup = make_scope_guard([&]() { close_fd(server_socket); });

    auto request = static_cast<const client_request_t*>(event_data);

    switch (request->data_type())
    {
    case request_data_t::table_scan:
    {
        auto type = static_cast<gaia_type_t>(request->data_as_table_scan()->type_id());

        start_stream_producer(server_socket, get_id_generator_for_type(type));

        break;
    }
    case request_data_t::index_scan:
    {
        auto request_data = request->data_as_index_scan();
        auto index_id = static_cast<gaia_id_t>(request_data->index_id());
        auto txn_id = static_cast<gaia_txn_id_t>(request_data->txn_id());
        auto query_type = request_data->query_type();
        auto index = id_to_index(index_id);

        ASSERT_INVARIANT(index != nullptr, "Cannot find index!");

        switch (query_type)
        {
        case index_query_t::NONE:
            start_stream_producer(server_socket, index->generator(txn_id));
            break;
        case index_query_t::index_point_read_query_t:
        case index_query_t::index_equal_range_query_t:
        {
            index::index_key_t key;
            {
                // Create local snapshot to query catalog for key serialization schema.
                bool apply_logs = true;
                create_local_snapshot(apply_logs);
                auto cleanup_local_snapshot = make_scope_guard([]() { s_local_snapshot_locators.close(); });

                if (query_type == index_query_t::index_point_read_query_t)
                {
                    auto query = request_data->query_as_index_point_read_query_t();
                    auto key_buffer = data_read_buffer_t(*(query->key()));
                    key = index::index_builder_t::deserialize_key(index_id, key_buffer);
                }
                else
                {
                    auto query = request_data->query_as_index_equal_range_query_t();
                    auto key_buffer = data_read_buffer_t(*(query->key()));
                    key = index::index_builder_t::deserialize_key(index_id, key_buffer);
                }
            }
            start_stream_producer(server_socket, index->equal_range_generator(txn_id, key));
            break;
        }
        default:
            ASSERT_UNREACHABLE(c_message_unexpected_query_type);
        }

        break;
    }
    default:
        ASSERT_UNREACHABLE(c_message_unexpected_request_data_type);
    }

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
    size_t log_fds_to_apply_count)
{
    flatbuffers::Offset<server_reply_t> server_reply;
    const auto transaction_info = Createtransaction_info_t(builder, txn_id, log_fds_to_apply_count);
    server_reply = Createserver_reply_t(
        builder, event, old_state, new_state, reply_data_t::transaction_info, transaction_info.Union());
    const auto message = Createmessage_t(builder, any_message_t::reply, server_reply.Union());
    builder.Finish(message);
}

void server_t::clear_shared_memory()
{
    data_mapping_t::close(c_data_mappings);
    s_local_snapshot_locators.close();
    s_chunk_manager.release();
}

// To avoid synchronization, we assume that this method is only called when
// no sessions exist and the server is not accepting any new connections.
void server_t::init_shared_memory()
{
    // The listening socket must not be open.
    ASSERT_PRECONDITION(s_listening_socket == -1, "Listening socket should not be open!");

    // Initialize global data structures.
    txn_metadata_t::init_txn_metadata_map();

    // Initialize watermarks.
    for (auto& elem : s_watermarks)
    {
        std::atomic_init(&elem, c_invalid_gaia_txn_id);
    }

    // We may be reinitializing the server upon receiving a SIGHUP.
    clear_shared_memory();

    // Clear all shared memory if an exception is thrown.
    auto cleanup_memory = make_scope_guard([]() { clear_shared_memory(); });

    // Validate shared memory mapping definitions and assert that mappings are not made yet.
    data_mapping_t::validate(c_data_mappings, std::size(c_data_mappings));
    for (auto data_mapping : c_data_mappings)
    {
        ASSERT_INVARIANT(!data_mapping.is_set(), "Memory should be unmapped");
    }

    // s_shared_locators uses sizeof(gaia_offset_t) * c_max_locators = 32GB of virtual address space.
    //
    // s_shared_data uses (64B) * c_max_locators = 256GB of virtual address space.
    //
    // s_shared_id_index uses (32B) * c_max_locators = 128GB of virtual address space
    // (assuming 4-byte alignment). We could eventually shrink this to
    // 4B/locator (assuming 4-byte locators), or 16GB, if we can assume that
    // gaia_ids are sequentially allocated and seldom deleted, so we can just
    // use an array of locators indexed by gaia_id.
    data_mapping_t::create(c_data_mappings, s_server_conf.instance_name().c_str());

    bool initializing = true;
    init_memory_manager(initializing);

    // Create snapshot for db recovery and index population.
    bool apply_logs = false;
    create_local_snapshot(apply_logs);

    // Populate shared memory from the persistent log and snapshot.
    recover_db();

    // Initialize indexes.
    init_indexes();

    // Done with local snapshot.
    s_local_snapshot_locators.close();

    cleanup_memory.dismiss();
}

void server_t::init_memory_manager(bool initializing)
{
    if (initializing)
    {
        // This is only called by the main thread, to prepare for recovery.
        s_memory_manager.initialize(
            reinterpret_cast<uint8_t*>(s_shared_data.data()->objects),
            sizeof(s_shared_data.data()->objects));

        memory_manager::chunk_offset_t chunk_offset = s_memory_manager.allocate_chunk();
        if (chunk_offset == c_invalid_chunk_offset)
        {
            throw memory_allocation_error("Memory manager ran out of memory during call to allocate_chunk().");
        }
        s_chunk_manager.initialize(chunk_offset);
    }
    else
    {
        // This is called by server-side session threads, to use in GC.
        // These threads perform no allocations, so they do not need to
        // initialize their chunk manager with an allocated chunk.
        s_memory_manager.load(
            reinterpret_cast<uint8_t*>(s_shared_data.data()->objects),
            sizeof(s_shared_data.data()->objects));
    }
}

void server_t::deallocate_object(gaia_offset_t offset)
{
    // First extract the chunk ID from the offset, so we know which chunks are
    // candidates for deallocation.
    memory_manager::chunk_offset_t chunk_offset = memory_manager::chunk_from_offset(offset);

    memory_manager::chunk_manager_t chunk_manager;
    chunk_manager.load(chunk_offset);

    // We need to read the chunk version *before* we deallocate the object, to
    // ensure that the chunk hasn't already been deallocated and reused before
    // we read the version!
    memory_manager::chunk_version_t version = chunk_manager.get_version();

    // Cache this chunk and its current version for later deallocation.
    // REVIEW: This could be changed to use contains() after C++20.
    if (s_map_gc_chunks_to_versions.count(chunk_offset) == 0)
    {
        s_map_gc_chunks_to_versions.insert({chunk_offset, version});
    }
    else
    {
        // If this GC task already cached this chunk, then the versions must match!
        memory_manager::chunk_version_t cached_version = s_map_gc_chunks_to_versions[chunk_offset];
        ASSERT_INVARIANT(version == cached_version, "Chunk version must match cached chunk version!");
    }

    // Delegate deallocation of the object to the chunk manager.
    chunk_manager.deallocate(offset);
}

// Initialize indexes on startup.
void server_t::init_indexes()
{
    // No data to index-- nothing to do here.
    if (s_server_conf.persistence_mode() == server_config_t::persistence_mode_t::e_disabled)
    {
        return;
    }

    auto cleanup = make_scope_guard([]() { end_startup_txn(); });
    // Allocate new txn id for initializing indexes.
    begin_startup_txn();

    gaia_locator_t locator = c_invalid_gaia_locator;
    gaia_locator_t last_locator = s_shared_counters.data()->last_locator;

    // Create initial index data structures.
    for (const auto& table : catalog_core_t::list_tables())
    {
        for (const auto& index : catalog_core_t::list_indexes(table.id()))
        {
            index::index_builder_t::create_empty_index(index);
        }
    }

    while (++locator && locator <= last_locator)
    {
        auto obj = locator_to_ptr(locator);

        // Skip system objects -- they are not indexed.
        if (is_system_object(obj->type))
        {
            continue;
        }

        gaia_id_t type_record_id
            = type_id_mapping_t::instance().get_record_id(obj->type);

        if (type_record_id == c_invalid_gaia_id)
        {
            // Orphaned object detected. We continue instead of throw here because of GAIAPLAT-1276.
            // This should be reverted once we no longer orphan objects during a DROP operation.
            std::cerr << "Cannot find type for object " << obj->id << " in the catalog!";
            continue;
        }

        for (const auto& index : catalog_core_t::list_indexes(type_record_id))
        {
            index::index_builder_t::populate_index(index.id(), obj->type, locator);
        }
    }
}

// On commit, update in-memory-indexes to reflect logged operations.
void server_t::update_indexes_from_txn_log()
{
    bool replay_logs = true;

    create_local_snapshot(replay_logs);
    auto cleanup_local_snapshot = make_scope_guard([]() { s_local_snapshot_locators.close(); });
    index::index_builder_t::update_indexes_from_logs(*s_log.data(), s_server_conf.skip_catalog_integrity_checks());
}

void server_t::recover_db()
{
    // If persistence is disabled, then this is a no-op.
    if (s_server_conf.persistence_mode() != persistence_mode_t::e_disabled)
    {
        // We could get here after a server reset with '--persistence disabled-after-recovery',
        // in which case we need to recover from the original persistent image.
        if (!s_persistent_store)
        {
            s_persistent_store = std::make_unique<gaia::db::persistent_store_manager>(get_counters(), s_server_conf.data_dir());
            if (s_server_conf.persistence_mode() == persistence_mode_t::e_reinitialized_on_startup)
            {
                s_persistent_store->destroy_persistent_store();
            }
            s_persistent_store->open();
            s_persistent_store->recover();
        }
    }

    // If persistence is disabled after recovery, then destroy the RocksDB
    // instance.
    if (s_server_conf.persistence_mode() == persistence_mode_t::e_disabled_after_recovery)
    {
        s_persistent_store.reset();
    }
}

gaia_txn_id_t server_t::begin_startup_txn()
{
    // Reserve an index in the safe_ts array, so the main thread can execute
    // post-commit maintenance tasks after the recovery txn commits.
    bool reservation_succeeded = reserve_safe_ts_index();
    // The reservation must have succeeded because we are the first thread to
    // reserve an index.
    ASSERT_POSTCONDITION(reservation_succeeded, "The main thread cannot fail to reserve a safe_ts index!");
    s_txn_id = txn_metadata_t::register_begin_ts();
    return s_txn_id;
}

void server_t::end_startup_txn()
{
    // Use a local variable to ensure cleanup in case of an error.
    mapped_log_t log;

    log.create(gaia_fmt::format("{}{}:{}", c_gaia_internal_txn_log_prefix, s_server_conf.instance_name(), s_txn_id).c_str());

    // Update the log header with our begin timestamp and initialize it to empty.
    log.data()->begin_ts = s_txn_id;
    log.data()->record_count = 0;

    // Claim ownership of the log fd from the mapping object.
    // The empty log will be freed by a regular GC task.
    int log_fd = log.unmap_truncate_seal_fd();

    // Register this txn under a new commit timestamp.
    gaia_txn_id_t commit_ts = txn_metadata_t::register_commit_ts(s_txn_id, log_fd);
    // Mark this txn as submitted.
    txn_metadata_t::set_active_txn_submitted(s_txn_id, commit_ts);
    // Mark this txn as committed.
    txn_metadata_t::update_txn_decision(commit_ts, true);
    // Mark this txn durable if persistence is enabled.
    if (s_persistent_store)
    {
        txn_metadata_t::set_txn_durable(commit_ts);
    }

    perform_maintenance();

    s_txn_id = c_invalid_gaia_txn_id;

    // The main thread no longer needs to perform any operations requiring a
    // safe_ts index.
    release_safe_ts_index();
}

// Create a thread-local snapshot from the shared locators.
void server_t::create_local_snapshot(bool apply_logs)
{
    ASSERT_PRECONDITION(!s_local_snapshot_locators.is_set(), "Local snapshot is already mapped!");
    bool manage_fd = false;
    bool is_shared = false;

    if (apply_logs)
    {
        ASSERT_PRECONDITION(
            s_txn_id != c_invalid_gaia_txn_id && txn_metadata_t::is_txn_active(s_txn_id),
            "create_local_snapshot() must be called from within an active transaction!");
        std::vector<int> txn_log_fds;
        get_txn_log_fds_for_snapshot(s_txn_id, txn_log_fds);
        auto cleanup_log_fds = make_scope_guard([&]() {
            // Close all the duplicated log fds in the buffer.
            for (auto& fd : txn_log_fds)
            {
                // Each log fd should still be valid.
                ASSERT_INVARIANT(is_fd_valid(fd), "Invalid fd!");
                close_fd(fd);
            }
        });

        // Open a private locator mmap for the current thread.
        s_local_snapshot_locators.open(s_shared_locators.fd(), manage_fd, is_shared);

        // Apply txn_logs for the snapshot.
        for (const auto& it : txn_log_fds)
        {
            mapped_log_t txn_log;
            txn_log.open(it);
            apply_log_to_locators(s_local_snapshot_locators.data(), txn_log.data());
        }

        // Apply s_log to the local snapshot, if any.
        if (s_log.is_set())
        {
            apply_log_to_locators(s_local_snapshot_locators.data(), s_log.data());
        }
    }
    else
    {
        s_local_snapshot_locators.open(s_shared_locators.fd(), manage_fd, is_shared);
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

    std::cerr << "Caught signal '" << ::strsignal(signum) << "'." << std::endl;

    signal_eventfd_multiple_threads(s_server_shutdown_eventfd);
}

void server_t::init_listening_socket(const std::string& socket_name)
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
    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;

    // The socket name (minus its null terminator) needs to fit into the space
    // in the server address structure after the prefix null byte.
    ASSERT_INVARIANT(
        socket_name.size() <= sizeof(server_addr.sun_path) - 1,
        "Socket name '" + socket_name + "' is too long!");

    // We prepend a null byte to the socket name so the address is in the
    // (Linux-exclusive) "abstract namespace", i.e., not bound to the
    // filesystem.
    ::strncpy(&server_addr.sun_path[1], socket_name.c_str(), sizeof(server_addr.sun_path) - 1);

    // Bind the socket to the address and start listening for connections.
    // The socket name is not null-terminated in the address structure, but
    // we need to add an extra byte for the null byte prefix.
    socklen_t server_addr_size = sizeof(server_addr.sun_family) + 1 + ::strlen(&server_addr.sun_path[1]);
    if (-1 == ::bind(listening_socket, reinterpret_cast<struct sockaddr*>(&server_addr), server_addr_size))
    {
        // REVIEW: Identify other common errors that should have user-friendly error messages.
        if (errno == EADDRINUSE)
        {
            std::cerr << "ERROR: bind() failed! - " << (::strerror(errno)) << std::endl;
            std::cerr
                << "The " << c_db_server_name
                << " cannot start because another instance is already running."
                << std::endl
                << "Stop any instances of the server and try again."
                << std::endl;
            exit(1);
        }

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
    // https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1253
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

void server_t::client_dispatch_handler(const std::string& socket_name)
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
    init_listening_socket(socket_name);
    // We close the listening socket before waiting for session threads to exit,
    // so no new sessions can be established while we wait for all session
    // threads to exit (we assume they received the same server shutdown
    // notification that we did).
    auto listener_cleanup = make_scope_guard([&]() { close_fd(s_listening_socket); });

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
        epoll_event ev{};
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

                if (s_session_threads.size() >= c_session_limit
                    || !authenticate_client_socket(session_socket))
                {
                    // The connecting client will get ECONNRESET on their first
                    // read from this socket.
                    close_fd(session_socket);
                    continue;
                }

                // First reap any session threads that have terminated (to
                // avoid memory and system resource leaks).
                reap_exited_threads(s_session_threads);

                // Create session thread.
                s_session_threads.emplace_back(session_handler, session_socket);
            }
            else if (ev.data.fd == s_server_shutdown_eventfd)
            {
                consume_eventfd(s_server_shutdown_eventfd);
                return;
            }
            else
            {
                // We don't monitor any other fds.
                ASSERT_UNREACHABLE(c_message_unexpected_fd);
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

    // Initialize this thread's memory manager.
    bool initializing = false;
    init_memory_manager(initializing);

    // Reserve an index in the safe_ts array. If this fails (because all indexes
    // are currently claimed by sessions), then immediately close the socket, so
    // the client throws a `peer_disconnected` exception and rethrows a
    // `session_limit_exceeded` exception.
    // REVIEW: When we have a way to marshal exceptions to the client, we should
    // directly ensure that `session_limit_exceeded` is thrown in this case.
    if (!reserve_safe_ts_index())
    {
        return;
    }

    auto safe_ts_index_cleanup = make_scope_guard([&]() {
        // Release this thread's index in the safe_ts array.
        release_safe_ts_index();
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
        epoll_event ev{};
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
        signal_eventfd_multiple_threads(s_session_shutdown_eventfd);

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
        uint8_t msg_buf[c_max_msg_size]{0};

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
                    std::cerr << "Client socket error: " << ::strerror(error) << std::endl;
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
                ASSERT_UNREACHABLE(c_message_unexpected_fd);
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
                std::cerr << "Client socket error: " << e.what() << std::endl;
                s_session_shutdown = true;
            }
        }
    }
}

template <typename T_element>
void server_t::stream_producer_handler(
    int stream_socket, int cancel_eventfd, std::shared_ptr<generator_t<T_element>> generator_fn)
{
    auto socket_cleanup = make_scope_guard([&]() {
        // We can rely on close_fd() to perform the equivalent of shutdown(SHUT_RDWR),
        // because we hold the only fd pointing to this socket.
        close_fd(stream_socket);
    });

    // Verify that the socket is the correct type for the semantics we assume.
    check_socket_type(stream_socket, SOCK_SEQPACKET);

    // Check that our stream socket is non-blocking (so we don't accidentally block in write()).
    ASSERT_PRECONDITION(is_non_blocking(stream_socket), "Stream socket is in blocking mode!");

    int epoll_fd = ::epoll_create1(0);
    if (epoll_fd == -1)
    {
        throw_system_error(c_message_epoll_create1_failed);
    }
    auto epoll_cleanup = make_scope_guard([&]() { close_fd(epoll_fd); });

    // We poll for write availability of the stream socket in level-triggered mode,
    // and only write at most one buffer of data before polling again, to avoid read
    // starvation of the cancellation eventfd.
    epoll_event sock_ev{};
    sock_ev.events = EPOLLOUT;
    sock_ev.data.fd = stream_socket;
    if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stream_socket, &sock_ev))
    {
        throw_system_error(c_message_epoll_ctl_failed);
    }

    epoll_event cancel_ev{};
    cancel_ev.events = EPOLLIN;
    cancel_ev.data.fd = cancel_eventfd;
    if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cancel_eventfd, &cancel_ev))
    {
        throw_system_error(c_message_epoll_ctl_failed);
    }

    epoll_event events[2];
    bool producer_shutdown = false;
    bool disabled_writable_notification = false;

    // The userspace buffer that we use to construct a batch datagram message.
    std::vector<T_element> batch_buffer;

    // We need to call reserve() rather than the "sized" constructor to avoid changing size().
    batch_buffer.reserve(c_stream_batch_size);

    auto gen_iter = generator_iterator_t<T_element>(std::move(generator_fn));

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
                    std::cerr << "Stream socket error: '" << ::strerror(error) << "'." << std::endl;
                    producer_shutdown = true;
                }
                else if (ev.events & EPOLLHUP)
                {
                    // This flag is unmaskable, so we don't need to register for it.
                    // We should get this when the client has closed its end of the socket.
                    ASSERT_INVARIANT(!(ev.events & EPOLLERR), c_message_epollerr_flag_should_not_be_set);
                    producer_shutdown = true;
                }
                else if (ev.events & EPOLLOUT)
                {
                    ASSERT_INVARIANT(
                        !disabled_writable_notification,
                        "Received write readiness notification on socket after deregistering from EPOLLOUT!");

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
                            std::cerr << "Stream socket error: '" << ::strerror(errno) << "'." << std::endl;
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
                        // Unintuitively, after we call shutdown(SHUT_WR), the
                        // socket is always writable, because a write will never
                        // block, but any write will return EPIPE. Therefore, we
                        // unregister the socket for writable notifications
                        // after we call shutdown(SHUT_WR). We should now only
                        // be notified (with EPOLLHUP/EPOLLERR) when the client
                        // closes the socket, so we can close our end of the
                        // socket and terminate the thread.
                        epoll_event ev{};
                        // We're only interested in EPOLLHUP/EPOLLERR
                        // notifications, and we don't need to register for
                        // those.
                        ev.events = 0;
                        ev.data.fd = stream_socket;
                        if (-1 == ::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, stream_socket, &ev))
                        {
                            throw_system_error(c_message_epoll_ctl_failed);
                        }
                        disabled_writable_notification = true;
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
                ASSERT_UNREACHABLE(c_message_unexpected_fd);
            }
        }
    }
}

template <typename T_element>
void server_t::start_stream_producer(int stream_socket, std::shared_ptr<generator_t<T_element>> generator_fn)
{
    // First reap any owned threads that have terminated (to avoid memory and
    // system resource leaks).
    reap_exited_threads(s_session_owned_threads);

    // Create stream producer thread.
    s_session_owned_threads.emplace_back(
        stream_producer_handler<T_element>, stream_socket, s_session_shutdown_eventfd, std::move(generator_fn));
}

std::shared_ptr<generator_t<gaia_id_t>> server_t::get_id_generator_for_type(gaia_type_t type)
{
    return std::make_shared<type_generator_t>(type, s_txn_id);
}

void server_t::validate_txns_in_range(gaia_txn_id_t start_ts, gaia_txn_id_t end_ts)
{
    // Scan txn table entries from start_ts to end_ts.
    // Seal any uninitialized entries and validate any undecided txns.
    for (gaia_txn_id_t ts = start_ts; ts < end_ts; ++ts)
    {
        // Fence off any txns that have allocated a commit_ts between start_ts
        // and end_ts but have not yet registered a commit_ts metadata entry in
        // the txn table.
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
        for (gaia_txn_id_t ts = txn_metadata_t::get_begin_ts_from_commit_ts(commit_ts) + 1; ts < commit_ts; ++ts)
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
    for (gaia_txn_id_t ts = txn_metadata_t::get_begin_ts_from_commit_ts(commit_ts) + 1; ts < commit_ts; ++ts)
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
bool server_t::advance_watermark(watermark_type_t watermark, gaia_txn_id_t ts)
{
    gaia_txn_id_t last_watermark_ts = get_watermark(watermark);
    do
    {
        // NB: last_watermark_ts is an inout argument holding the previous value
        // on failure!
        if (ts <= last_watermark_ts)
        {
            return false;
        }

    } while (!get_watermark_entry(watermark).compare_exchange_weak(last_watermark_ts, ts));

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
        txn_log.data()->begin_ts == txn_metadata_t::get_begin_ts_from_commit_ts(commit_ts),
        "txn log begin_ts must match begin_ts reference in commit_ts metadata!");

    // Update the shared locator view with each redo version (i.e., the
    // version created or updated by the txn). This is safe as long as the
    // committed txn being applied has commit_ts older than the oldest
    // active txn's begin_ts (so it can't overwrite any versions visible in
    // that txn's snapshot). This update is non-atomic because log application
    // is idempotent and therefore a txn log can be re-applied over the same
    // txn's partially-applied log during snapshot reconstruction.
    apply_log_to_locators(s_shared_locators.data(), txn_log.data());
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

        // If we're gc-ing the old version of an object that is being deleted,
        // then request the deletion of its locator from the corresponding record list.
        if (!deallocate_new_offsets && txn_log->log_records[i].new_offset == c_invalid_gaia_offset)
        {
            // Get the old object data to extract its type.
            db_object_t* db_object = offset_to_ptr(txn_log->log_records[i].old_offset);

            // Retrieve the record_list_t instance corresponding to the type.
            std::shared_ptr<record_list_t> record_list = record_list_manager_t::get()->get_record_list(db_object->type);

            // Request the deletion of the record corresponding to the object.
            record_list->request_deletion(txn_log->log_records[i].locator);
        }

        if (offset_to_free)
        {
            deallocate_object(offset_to_free);
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
// TODO: Decommit physical pages backing the txn table for addresses preceding
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

    // Find a timestamp at which we can safely truncate the txn table.
    // TODO: Actually reclaim memory from the truncated prefix of the txn table.
    truncate_txn_table();
}

void server_t::apply_txn_logs_to_shared_view()
{
    // First get a snapshot of the timestamp counter for an upper bound on
    // the scan (we don't know yet if this is a begin_ts or commit_ts).
    gaia_txn_id_t last_allocated_ts = get_last_txn_id();

    // Now get a snapshot of the pre-apply watermark,
    // for a lower bound on the scan.
    gaia_txn_id_t pre_apply_watermark = get_watermark(watermark_type_t::pre_apply);

    // Scan from the saved pre-apply watermark to the last known timestamp,
    // and apply all committed txn logs from the longest prefix of decided
    // txns that does not overlap with the conflict window of any undecided
    // txn. Advance the pre-apply watermark before applying the txn log
    // of a committed txn, and advance the post-apply watermark after
    // applying the txn log.
    for (gaia_txn_id_t ts = pre_apply_watermark + 1; ts <= last_allocated_ts; ++ts)
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
                && txn_metadata_t::is_txn_validating(txn_metadata_t::get_commit_ts_from_begin_ts(ts)))
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
        if (get_watermark(watermark_type_t::pre_apply) != prev_ts || get_watermark(watermark_type_t::post_apply) != prev_ts)
        {
            break;
        }

        if (!advance_watermark(watermark_type_t::pre_apply, ts))
        {
            // If another thread has already advanced the watermark ahead of
            // this ts, we abort advancing it further.
            ASSERT_INVARIANT(
                get_watermark(watermark_type_t::pre_apply) > pre_apply_watermark,
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
        bool has_advanced_watermark = advance_watermark(watermark_type_t::post_apply, ts);

        // No other thread should be able to advance the post-apply watermark,
        // because only one thread can advance the pre-apply watermark to this
        // timestamp.
        ASSERT_INVARIANT(has_advanced_watermark, "Couldn't advance the post-apply watermark!");
    }
}

void server_t::gc_applied_txn_logs()
{
    // Ensure we clean up our cached chunk IDs when we exit this task.
    auto cleanup_fd = make_scope_guard([&]() { s_map_gc_chunks_to_versions.clear(); });

    // First get a snapshot of the post-apply watermark, for an upper bound on the scan.
    gaia_txn_id_t post_apply_watermark = get_watermark(watermark_type_t::post_apply);

    // Now get a snapshot of the post-GC watermark, for a lower bound on the scan.
    gaia_txn_id_t post_gc_watermark = get_watermark(watermark_type_t::post_gc);

    // Scan from the post-GC watermark to the post-apply watermark,
    // executing GC on any commit_ts if the log fd is valid (and the durable
    // flag is set if persistence is enabled). (If we fail to invalidate the log
    // fd, we abort the scan to avoid contention.) When GC is complete, set the
    // TXN_GC_COMPLETE flag on the txn metadata and continue.
    for (gaia_txn_id_t ts = post_gc_watermark + 1; ts <= post_apply_watermark; ++ts)
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
            bool is_persistence_enabled = (s_server_conf.persistence_mode() != persistence_mode_t::e_disabled)
                && (s_server_conf.persistence_mode() != persistence_mode_t::e_disabled_after_recovery);

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

    // Now deallocate any unused chunks that have already been retired.
    // TODO: decommit unused pages (https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1446)
    for (auto& entry : s_map_gc_chunks_to_versions)
    {
        memory_manager::chunk_offset_t chunk_offset = entry.first;
        memory_manager::chunk_version_t chunk_version = entry.second;
        memory_manager::chunk_manager_t chunk_manager;
        chunk_manager.load(chunk_offset);
        chunk_manager.try_deallocate_chunk(chunk_version);
        chunk_manager.release();
    }
}

void server_t::truncate_txn_table()
{
    // First catch up the post-GC watermark.
    // Unlike log application, we don't try to perform GC and advance the
    // post-GC watermark in a single scan, because log application is strictly
    // sequential, while GC is sequentially initiated but concurrently executed.

    // Get a snapshot of the post-apply watermark, for an upper bound on the scan.
    gaia_txn_id_t post_apply_watermark = get_watermark(watermark_type_t::post_apply);

    // Get a snapshot of the post-GC watermark, for a lower bound on the scan.
    gaia_txn_id_t post_gc_watermark = get_watermark(watermark_type_t::post_gc);

    // Scan from the post-GC watermark to the post-apply watermark,
    // advancing the post-GC watermark on any begin_ts, or any commit_ts that
    // has the TXN_GC_COMPLETE flag set. If TXN_GC_COMPLETE is unset on the
    // current commit_ts, abort the scan.
    for (gaia_txn_id_t ts = post_gc_watermark + 1; ts <= post_apply_watermark; ++ts)
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

        if (!advance_watermark(watermark_type_t::post_gc, ts))
        {
            // If another thread has already advanced the post-GC watermark
            // ahead of this ts, we abort advancing it further.
            ASSERT_INVARIANT(
                get_watermark(watermark_type_t::post_gc) > post_gc_watermark,
                "The watermark must have advanced if advance_watermark() failed!");

            break;
        }

        // Get a snapshot of the pre-truncate watermark before advancing it.
        gaia_txn_id_t prev_pre_truncate_watermark = get_watermark(watermark_type_t::pre_truncate);

        // Compute a safe truncation timestamp.
        gaia_txn_id_t new_pre_truncate_watermark = get_safe_truncation_ts();

        // Abort if the safe truncation timestamp does not exceed the current
        // pre-truncate watermark.
        // NB: It is expected that the safe truncation timestamp can be behind the
        // pre-truncate watermark, because some published (but not fully reserved)
        // timestamps may have been behind the pre-truncate watermark when they
        // were published (and will later fail validation).
        if (new_pre_truncate_watermark <= prev_pre_truncate_watermark)
        {
            return;
        }

        // Try to advance the pre-truncate watermark.
        if (!advance_watermark(watermark_type_t::pre_truncate, new_pre_truncate_watermark))
        {
            // Abort if another thread has concurrently advanced the pre-truncate
            // watermark, to avoid contention.
            ASSERT_INVARIANT(
                get_watermark(watermark_type_t::pre_truncate) > prev_pre_truncate_watermark,
                "The watermark must have advanced if advance_watermark() failed!");

            return;
        }
    }
}

void server_t::txn_rollback(bool client_disconnected)
{
    // Set our txn status to TXN_TERMINATED.
    // NB: this must be done before calling perform_maintenance()!
    txn_metadata_t::set_active_txn_terminated(s_txn_id);

    // Update watermarks and perform associated maintenance tasks.
    perform_maintenance();

    // Directly free the final allocation recorded in chunk metadata if it is
    // absent from the txn log (due to a crashed session), and retire the chunk
    // owned by the client session when it crashed.
    if (client_disconnected)
    {
        // Load the crashed session's chunk, so we can directly free the final
        // allocation if necessary.
        // BUG (GAIAPLAT-1489): this will only work if we're inside a txn when
        // the session crashes. Otherwise, we'll leak the chunk and it will be
        // orphaned forever (unless we somehow GC orphaned chunks).
        memory_manager::chunk_offset_t chunk_offset = s_log.data()->current_chunk;
        if (chunk_offset != c_invalid_chunk_offset)
        {
            s_chunk_manager.load(chunk_offset);

            // Get final allocation from chunk metadata, so we can check to be sure
            // it's present in our txn log.
            gaia_offset_t last_allocated_offset = s_chunk_manager.last_allocated_offset();

            // If we haven't logged the final allocation, then deallocate it directly.
            if (last_allocated_offset != c_invalid_gaia_offset)
            {
                bool is_log_empty = (s_log.data()->record_count == 0);

                // If we have no log records, but we do have a final allocation, then deallocate it directly.
                bool should_deallocate_directly = is_log_empty;

                // If we do have a non-empty log, then check if the final record matches the final allocation.
                // if (!is_log_empty)
                // {
                // BUG (GAIAPLAT-1490): I removed the original logic here
                // because it was incorrect. We can afford to leak the final
                // allocation when a session crashes in the middle of a txn
                // for now, until we implement this properly.
                // }

                if (should_deallocate_directly)
                {
                    s_chunk_manager.deallocate(last_allocated_offset);
                }
            }

            // Get the session's chunk version for safe deallocation.
            memory_manager::chunk_version_t version = s_chunk_manager.get_version();

            // Now retire the chunk.
            s_chunk_manager.retire_chunk(version);

            // We don't strictly need this since the session thread is about to exit,
            // but it's good hygiene to clear unused thread-locals.
            s_chunk_manager.release();
        }
    }

    // This session now has no active txn.
    s_txn_id = c_invalid_gaia_txn_id;

    // Claim ownership of the log fd from the mapping object.
    bool is_log_empty = (s_log.data()->record_count == 0);
    int log_fd = s_log.unmap_truncate_seal_fd();

    // We need to unconditionally close the log fd because we're not registering
    // it in a txn metadata entry, so it won't be closed by GC.
    auto cleanup_log_fd = make_scope_guard([&]() { close_fd(log_fd); });

    // Free any deallocated objects (don't bother for read-only txns).
    if (!is_log_empty)
    {
        gc_txn_log_from_fd(log_fd, false);
    }
}

void server_t::perform_pre_commit_work_for_txn()
{
    ASSERT_PRECONDITION(s_log.is_set(), c_message_uninitialized_log_fd);

    // Process the txn log to update record lists.
    for (size_t i = 0; i < s_log.data()->record_count; ++i)
    {
        txn_log_t::log_record_t* lr = &(s_log.data()->log_records[i]);

        // In case of insertions, we want to update the record list for the object's type.
        // We do this after updating the shared locator view, so we can access the new object's data.
        if (lr->old_offset == c_invalid_gaia_offset)
        {
            gaia_locator_t locator = lr->locator;
            gaia_offset_t offset = lr->new_offset;

            ASSERT_INVARIANT(
                offset != c_invalid_gaia_offset, "An unexpected invalid object offset was found in the log record!");

            db_object_t* db_object = offset_to_ptr(offset);
            std::shared_ptr<record_list_t> record_list = record_list_manager_t::get()->get_record_list(db_object->type);
            record_list->add(locator);
        }
    }

    update_indexes_from_txn_log();
}

// Sort all txn log records by locator. This enables us to use fast binary
// search and binary merge algorithms for conflict detection.
void server_t::sort_log()
{
    ASSERT_PRECONDITION(s_log.is_set(), c_message_uninitialized_log_fd);

    // We use stable_sort() to preserve the temporal order of multiple updates
    // to the same locator.
    std::stable_sort(
        &s_log.data()->log_records[0],
        &s_log.data()->log_records[s_log.data()->record_count],
        [](const txn_log_t::log_record_t& lhs, const txn_log_t::log_record_t& rhs) {
            return lhs.locator < rhs.locator;
        });
}

// Before this method is called, we have already received the log fd from the client
// and mmapped it.
// This method returns true for a commit decision and false for an abort decision.
bool server_t::txn_commit()
{
    // Perform pre-commit work.
    perform_pre_commit_work_for_txn();

    // Before registering the log, sort by locator for fast conflict detection.
    sort_log();

    // From now on, the txn log should be immutable.
    // Claim ownership of the log fd from the mapping object.
    int log_fd = s_log.unmap_truncate_seal_fd();

    // Register the committing txn under a new commit timestamp.
    gaia_txn_id_t commit_ts = submit_txn(s_txn_id, log_fd);

    // This is only used for persistence.
    std::string txn_name;

    if (s_persistent_store)
    {
        txn_name = s_persistent_store->begin_txn(s_txn_id);
        // Prepare log for transaction.
        // This is effectively asynchronous with validation, because if it takes
        // too long, then another thread may recursively validate this txn,
        // before the committing thread has a chance to do so.
        mapped_log_t log;
        log.open(log_fd);
        s_persistent_store->prepare_wal_for_write(log.data(), txn_name);
    }

    // Validate the txn against all other committed txns in the conflict window.
    bool is_committed = validate_txn(commit_ts);

    // Update the txn metadata with our commit decision.
    txn_metadata_t::update_txn_decision(commit_ts, is_committed);

    // Persist the commit decision.
    // REVIEW: We can return a decision to the client asynchronously with the
    // decision being persisted (because the decision can be reconstructed from
    // the durable log itself, without the decision record).
    if (s_persistent_store)
    {
        // Mark txn as durable in metadata so we can GC the txn log.
        // We only mark it durable after validation to simplify the
        // state transitions:
        // TXN_VALIDATING -> TXN_DECIDED -> TXN_DURABLE.
        txn_metadata_t::set_txn_durable(commit_ts);

        if (is_committed)
        {
            s_persistent_store->append_wal_commit_marker(txn_name);
        }
        else
        {
            s_persistent_store->append_wal_rollback_marker(txn_name);
        }
    }

    return is_committed;
}

bool server_t::reserve_safe_ts_index()
{
    ASSERT_PRECONDITION(
        s_safe_ts_index == c_invalid_safe_ts_index,
        "Expected this thread's safe_ts entry index to be invalid!");

    // Try to set the first unset bit in the "free safe_ts indexes" bitmap.
    size_t reserved_index = c_invalid_safe_ts_index;
    while (true)
    {
        reserved_index = memory_manager::find_first_unset_bit(
            s_safe_ts_reserved_indexes_bitmap.data(), s_safe_ts_reserved_indexes_bitmap.size());

        // If our scan doesn't find any unset bits, immediately return failure
        // rather than retrying the scan (otherwise this could lead to an
        // infinite loop).
        if (reserved_index == c_max_bit_index)
        {
            return false;
        }

        // If the CAS to set the bit fails, we need to retry the scan, even if
        // the bit was still unset when the CAS failed.
        if (memory_manager::try_set_bit_value(
                s_safe_ts_reserved_indexes_bitmap.data(),
                s_safe_ts_reserved_indexes_bitmap.size(),
                reserved_index, true))
        {
            break;
        }
    }

    // Set this thread's safe_ts entry index to the bit we set.
    s_safe_ts_index = reserved_index;

    return true;
}

void server_t::release_safe_ts_index()
{
    ASSERT_PRECONDITION(
        s_safe_ts_index != c_invalid_safe_ts_index,
        "Expected this thread's safe_ts entry index to be valid!");

    ASSERT_PRECONDITION(
        memory_manager::is_bit_set(
            s_safe_ts_reserved_indexes_bitmap.data(),
            s_safe_ts_reserved_indexes_bitmap.size(),
            s_safe_ts_index),
        "Expected the bit for this thread's safe_ts entry index to be set!");

    // Invalidate both of this thread's safe_ts entries.
    s_safe_ts_per_thread_entries[s_safe_ts_index][0] = c_invalid_gaia_txn_id;
    s_safe_ts_per_thread_entries[s_safe_ts_index][1] = c_invalid_gaia_txn_id;

    // Invalidate the thread-local copy of this entry's index before marking its
    // index "free".
    size_t safe_ts_index = s_safe_ts_index;
    s_safe_ts_index = c_invalid_safe_ts_index;

    // Clear the bit for this entry's index in the "free safe_ts indexes"
    // bitmap.
    memory_manager::safe_set_bit_value(
        s_safe_ts_reserved_indexes_bitmap.data(),
        s_safe_ts_reserved_indexes_bitmap.size(),
        safe_ts_index, false);
}

bool server_t::reserve_safe_ts(gaia_txn_id_t safe_ts)
{
    ASSERT_PRECONDITION(
        s_safe_ts_index != c_invalid_safe_ts_index,
        "Expected this thread's safe_ts entry index to be valid!");

    // The reservation of a "safe timestamp" is split into 2 steps:
    // "publication" and "validation". Publication makes the reserved timestamp
    // visible to all other threads (so it will be observed by a scan).
    // Validation ensures that the reserving thread cannot use a timestamp that
    // was published too late to avoid being overtaken by the pre-truncate
    // watermark.
    //
    // NB: We need to maintain visibility of this thread's previous reserved
    // timestamp in case validation fails. (We cannot just invalidate it before
    // validation and then restore it after validation fails, because then it
    // would not be visible to a concurrent scan.) We do that by maintaining
    // *two* entries for each thread, and only invalidating the previous
    // reserved timestamp after validation of the new reserved timestamp
    // succeeds. An invalid entry will be ignored by the scan algorithm in its
    // calculation of the minimum published timestamp. (If both entries are
    // valid, then the scan algorithm will just take the minimum. If the minimum
    // entry is obsolete, then the scan algorithm will just return a result that
    // is more conservative--i.e. smaller--than necessary, but still correct.)
    //
    // Find the last invalid entry. Since the current thread has exclusive write
    // access to these entries, we should not have two valid entries (any
    // obsolete entry should have been invalidated by a previous call to this
    // method).
    auto& entries = s_safe_ts_per_thread_entries[s_safe_ts_index];
    std::array<bool, s_safe_ts_per_thread_entries[0].size()> is_entry_valid{};
    for (size_t i = 0; i < entries.size(); ++i)
    {
        is_entry_valid[i] = (entries[i] != c_invalid_gaia_txn_id);
    }
    ASSERT_INVARIANT(
        !is_entry_valid[0] || !is_entry_valid[1],
        "At most one safe_ts entry for this thread should be valid!");

    // Is exactly one of the two entries valid?
    bool has_valid_entry = (is_entry_valid[0] ^ is_entry_valid[1]);

    // Arbitrarily pick the highest-indexed invalid entry.
    size_t invalid_entry_index = is_entry_valid[1] ? 0 : 1;

    // Speculatively publish the new safe_ts in a currently invalid entry.
    entries[invalid_entry_index] = safe_ts;

    // Validate that the new safe_ts does not lag the post-GC watermark
    // (equality is acceptable because the post-GC watermark is an inclusive
    // upper bound on the pre-truncate watermark, and the pre-truncate watermark
    // is an exclusive upper bound on the memory address range eligible for
    // reclaiming).
    //
    // An essential optimization (necessary for the safe_ts_t implementation):
    // skip validation for a safe_ts that is replacing a smaller previously
    // reserved safe_ts. Even if the new safe_ts is behind the post-GC
    // watermark, it cannot be behind the pre-truncate watermark, because the
    // smaller previously reserved safe_ts prevents the pre-truncate watermark
    // from advancing past the new safe_ts until the new safe_ts is observed by
    // a scan. If both safe_ts values are published, then a concurrent scan will
    // use the obsolete value, but that is benign: it can only cause a
    // smaller-than-necessary safe truncation timestamp to be returned.
    bool should_validate = true;
    if (has_valid_entry)
    {
        // The valid index must be the invalid index + 1 mod 2.
        size_t valid_entry_index = invalid_entry_index ^ 1;
        should_validate = (safe_ts < entries[valid_entry_index]);
    }

    if (should_validate && safe_ts < get_watermark(watermark_type_t::post_gc))
    {
        // If validation fails, invalidate this entry to revert to the
        // previously published entry.
        entries[invalid_entry_index] = c_invalid_gaia_txn_id;
        return false;
    }

    // Invalidate any previously published entry.
    if (has_valid_entry)
    {
        // The valid index must be the invalid index + 1 mod 2.
        size_t valid_entry_index = invalid_entry_index ^ 1;
        entries[valid_entry_index] = c_invalid_gaia_txn_id;
    }

    return true;
}

void server_t::release_safe_ts()
{
    ASSERT_PRECONDITION(
        s_safe_ts_index != c_invalid_safe_ts_index,
        "Expected this thread's safe_ts entry index to be valid!");

    // Find the index of the last valid entry. Since the current thread has
    // exclusive write access to these entries, and the contract is that this
    // method should only be called after reserve_safe_ts() returns success, we
    // should have exactly one valid entry. Also, any valid entry should be at
    // least as large as the pre-truncate watermark (otherwise we either failed
    // to invalidate the entry of a safe_ts that failed validation, or the scan
    // algorithm is incorrect).
    auto& entries = s_safe_ts_per_thread_entries[s_safe_ts_index];
    std::array<bool, s_safe_ts_per_thread_entries[0].size()> is_entry_valid{};
    for (size_t i = 0; i < entries.size(); ++i)
    {
        ASSERT_INVARIANT(
            (entries[i] == c_invalid_gaia_txn_id) || (entries[i] >= get_watermark(watermark_type_t::pre_truncate)),
            "A reserved safe_ts entry cannot lag the pre-truncate watermark!");

        is_entry_valid[i] = (entries[i] != c_invalid_gaia_txn_id);
    }

    ASSERT_INVARIANT(
        (is_entry_valid[0] ^ is_entry_valid[1]),
        "Exactly one safe_ts entry for this thread should be valid!");

    // Invalidate the previously published entry.
    size_t valid_entry_index = is_entry_valid[0] ? 0 : 1;
    entries[valid_entry_index] = c_invalid_gaia_txn_id;
}

gaia_txn_id_t server_t::get_safe_truncation_ts()
{
    // The algorithm (loosely based on Maged Michael's "Hazard Pointers: Safe
    // Memory Reclamation for Lock-Free Objects"):
    //
    // 1. Take a snapshot of the post-GC watermark. This is an upper bound on
    //    the "safe truncation timestamp" return value, and guarantees that the
    //    "safe truncation timestamp" will not exceed any concurrently reserved
    //    timestamp (which might not be visible to the scan).
    // 2. Scan the "published timestamps" array and compute the minimum of all
    //    valid timestamps observed.
    // 3. Take the minimum of the "minimum observed published timestamp" and the
    //    pre-scan snapshot of the post-GC watermark, and return that as the
    //    "safe truncation timestamp". (Note that this timestamp may actually be
    //    behind the current value of the pre-truncate watermark, but we won't
    //    detect that until we fail to advance the pre-truncate watermark to
    //    this timestamp. Any published timestamp that is already behind the
    //    pre-truncate watermark will fail validation.)
    //
    // REVIEW: Need to specify this algorithm in a model-checkable spec language
    // like TLA+.
    //
    // Proof of correctness (very informal):
    //
    // We wish to show that after reserve_safe_ts(safe_ts) has returned success,
    // the pre-truncate watermark can never exceed safe_ts.
    //
    // First, we need to show that get_safe_truncation_ts() cannot return a
    // timestamp greater than safe_ts. There are only two possible cases to
    // consider: either (1) the scan of published timestamps observed safe_ts,
    // or (2) it did not.
    //
    // 1. If safe_ts was observed by the scan, then we know that it is an upper
    //    bound on the return value of get_safe_truncation_ts(), so we are done.
    //
    // 2. If safe_ts was not observed by the scan, then the pre-scan snapshot of
    //    the post-GC watermark must have been taken before the validation-time
    //    snapshot of the post-GC watermark (because validation happens after
    //    publication, and the publication evidently did not happen before the
    //    scan started). Because validation succeeded, safe_ts is at least as
    //    large as the validation-time snapshot of the post-GC watermark, so
    //    (because watermarks are monotonically nondecreasing) it must also be
    //    at least as large as the pre-scan snapshot of the post-GC watermark.
    //    Because the pre-scan snapshot of the post-GC watermark is an upper
    //    bound on the return value of get_safe_truncation_ts(), safe_ts is as
    //    well, and we are done.
    //
    // Given that safe_ts is an upper bound on the return value of
    // get_safe_truncation_ts(), and the pre-truncate watermark can only be
    // advanced to a return value of get_safe_truncation_ts(), it follows that
    // safe_ts is always an upper bound on the pre-truncate watermark. QED.
    //
    // Schematically:
    // "pre-truncate watermark"
    // <= "pre-scan snapshot of post-GC watermark"
    // <= "publication-time value of post-GC watermark"
    // <= "validation-time snapshot of post-GC watermark"
    // <= "published and validated timestamp"

    // Take a snapshot of the post-GC watermark before the scan.
    gaia_txn_id_t pre_scan_post_gc_watermark = get_watermark(watermark_type_t::post_gc);

    // The post-GC watermark is an upper bound on the safe truncation timestamp.
    gaia_txn_id_t safe_truncation_ts = pre_scan_post_gc_watermark;

    // Scan the "published timestamps" array and compute the minimum of all
    // published timestamps observed. Then return the minimum of that result and
    // the pre-scan snapshot of the post-GC watermark.
    //
    // Note that a published timestamp may have already fallen behind the
    // current pre-truncate watermark (which will make it fail validation). In
    // that case, we will return a timestamp older than the current pre-truncate
    // watermark. There is no need to try to prevent this, because
    // advance_watermark() will fail, the current GC task will abort, and
    // another thread will try to advance the pre-truncate watermark.
    for (auto& entries : s_safe_ts_per_thread_entries)
    {
        for (gaia_txn_id_t entry : entries)
        {
            // Skip invalid entries.
            if (entry == c_invalid_gaia_txn_id)
            {
                continue;
            }

            // Update the minimum safe_ts.
            safe_truncation_ts = std::min(safe_truncation_ts, entry);
        }
    }

    // Return the minimum of the pre-scan snapshot of the post-GC watermark and
    // the smallest published timestamp that the scan observed.
    return safe_truncation_ts;
}

static bool is_system_compatible()
{
    std::cerr << std::endl;

    if (!is_little_endian())
    {
        std::cerr << "The Gaia Database Server does not support big-endian CPU architectures." << std::endl;
        return false;
    }

    if (!has_expected_page_size())
    {
        std::cerr << "The Gaia Database Server requires page size to be 4KB." << std::endl;
        return false;
    }

    uint64_t policy_id = check_overcommit_policy();
    const char* policy_desc = c_vm_overcommit_policies[policy_id].desc;
    if (policy_id != c_always_overcommit_policy_id)
    {
        std::cerr
            << "The current overcommit policy has a value of "
            << policy_id << " (" << policy_desc << ")."
            << std::endl;
    }

    if (policy_id == c_heuristic_overcommit_policy_id)
    {
        std::cerr
            << "The Gaia Database Server will run normally under this overcommit policy,"
            << " but may become unstable under rare conditions."
            << std::endl;
    }

    if (policy_id == c_never_overcommit_policy_id)
    {
        std::cerr
            << "The Gaia Database Server will not run under this overcommit policy."
            << std::endl;
    }

    if (policy_id != c_always_overcommit_policy_id)
    {
        std::cerr
            << std::endl
            << "To ensure stable performance under all conditions, we recommend"
            << " changing the overcommit policy to "
            << c_always_overcommit_policy_id << " ("
            << c_vm_overcommit_policies[c_always_overcommit_policy_id].desc << ")."
            << std::endl;

        std::cerr << R"(
To temporarily enable this policy, open a shell with root privileges and type the following command:

  echo 1 > /proc/sys/vm/overcommit_memory

To permanently enable this policy, open /etc/sysctl.conf in an editor with root privileges and add the line:

  vm.overcommit_memory=1

Save the file, and in a shell with root privileges type:

  sysctl -p
        )" << std::endl;
    }

    if (policy_id == c_never_overcommit_policy_id)
    {
        return false;
    }

    if (!check_vma_limit())
    {
        std::cerr << R"(
The Gaia Database Server requires a per-process virtual memory area limit of at least 65530.

To temporarily set the minimum virtual memory area limit, open a shell with root privileges and type the following command:

  echo 65530 > /proc/sys/vm/max_map_count

To permanently set the minimum virtual memory area limit, open /etc/sysctl.conf in an editor with root privileges and add the line:

  vm.max_map_count=65530

Save the file, and in a shell with root privileges type:

  sysctl -p
        )" << std::endl;
        return false;
    }

    if (!check_and_adjust_vm_limit())
    {
        std::cerr << R"(
The Gaia Database Server requires that the maximum possible virtual memory address space is available.

To temporarily enable the maximum virtual memory address space, open a shell with root privileges and type the following command:

  ulimit -v unlimited

To permanently enable the maximum virtual memory address space, open /etc/security/limits.conf in an editor with root privileges and add the following lines:

  * soft as unlimited
  * hard as unlimited

Note: For enhanced security, replace the wildcard '*' in these file entries with the user name of the account that is running the Gaia Database Server.

Save the file and start a new terminal session.
        )" << std::endl;
        return false;
    }

    if (!check_and_adjust_fd_limit())
    {
        std::cerr << R"(
The Gaia Database Server requires a per-process open file descriptor limit of at least 65535.

To temporarily set the minimum open file descriptor limit, open a shell with root privileges and type the following command:

  ulimit -n 65535

To permanently set the minimum open file descriptor limit, open /etc/security/limits.conf in an editor with root privileges and add the following lines:

  * soft nofile 65535
  * hard nofile 65535

Note: For enhanced security, replace the wildcard '*' in these file entries with the user name of the account that is running the Gaia Database Server.

Save the file and start a new terminal session.
        )" << std::endl;
        return false;
    }

    return true;
}

// This method must be run on the main thread
// (https://thomastrapp.com/blog/signal-handler-for-multithreaded-c++/).
void server_t::run(server_config_t server_conf)
{
    // First validate our system assumptions.
    if (!is_system_compatible())
    {
        std::cerr << "The Gaia Database Server is exiting due to an unsupported system configuration." << std::endl;
        std::exit(1);
    }

    // There can only be one thread running at this point, so this doesn't need synchronization.
    s_server_conf = server_conf;

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

        // Block handled signals in this thread and subsequently spawned threads.
        sigset_t handled_signals = mask_signals();

        // Launch signal handler thread.
        int caught_signal = 0;
        std::thread signal_handler_thread(signal_handler, handled_signals, std::ref(caught_signal));

        init_shared_memory();

        // Launch thread to listen for client connections and create session threads.
        std::thread client_dispatch_thread(client_dispatch_handler, server_conf.instance_name());

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
              && (server_conf.persistence_mode() == persistence_mode_t::e_disabled
                  || server_conf.persistence_mode() == persistence_mode_t::e_disabled_after_recovery)))
        {
            if (caught_signal == SIGHUP)
            {
                std::cerr << "Unable to reset the server because persistence is enabled, exiting." << std::endl;
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
