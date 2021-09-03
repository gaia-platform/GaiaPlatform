/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_client.hpp"

#include <unistd.h>

#include <functional>
#include <optional>
#include <thread>

#include <flatbuffers/flatbuffers.h>
#include <gaia_spdlog/fmt/fmt.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gaia/exceptions.hpp"

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
#include "messages_generated.h"
#include "predicate.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::db::messages;
using namespace gaia::db::memory_manager;
using namespace flatbuffers;
using namespace scope_guard;

int client_t::get_id_cursor_socket_for_type(gaia_type_t type)
{
    // Build the cursor socket request.
    FlatBufferBuilder builder;
    auto table_scan_info = Createtable_scan_info_t(builder, type);
    auto client_request = Createclient_request_t(builder, session_event_t::REQUEST_STREAM, request_data_t::table_scan, table_scan_info.Union());
    auto message = Createmessage_t(builder, any_message_t::request, client_request.Union());
    builder.Finish(message);

    client_messenger_t client_messenger;
    client_messenger.send_and_receive(s_session_socket, nullptr, 0, builder, 1);

    int stream_socket = client_messenger.received_fd(client_messenger_t::c_index_stream_socket);
    auto cleanup_stream_socket = make_scope_guard([&]() {
        close_fd(stream_socket);
    });

    const session_event_t event = client_messenger.server_reply()->event();
    ASSERT_INVARIANT(event == session_event_t::REQUEST_STREAM, c_message_unexpected_event_received);

    // Check that our stream socket is blocking (because we need to perform blocking reads).
    ASSERT_INVARIANT(!is_non_blocking(stream_socket), "Stream socket is not set to blocking!");

    cleanup_stream_socket.dismiss();
    return stream_socket;
}

std::function<std::optional<gaia_id_t>()>
client_t::augment_id_generator_for_type(gaia_type_t type, std::function<std::optional<gaia_id_t>()> id_generator)
{
    bool has_exhausted_id_generator = false;
    size_t log_index = 0;

    std::function<std::optional<gaia_id_t>()> augmented_id_generator
        = [type, id_generator, has_exhausted_id_generator, log_index]() mutable -> std::optional<gaia_id_t> {
        // First, we use the id_generator until it's exhausted.
        if (!has_exhausted_id_generator)
        {
            std::optional<gaia_id_t> id_opt = id_generator();
            if (id_opt)
            {
                return id_opt;
            }
            else
            {
                has_exhausted_id_generator = true;
            }
        }

        // Once the id_generator is exhausted, we start iterating over our transaction log.
        if (has_exhausted_id_generator)
        {
            while (log_index < s_log.data()->record_count)
            {
                txn_log_t::log_record_t* lr = &(s_log.data()->log_records[log_index++]);

                // Look for insertions of objects of the given data type and return their gaia_id.
                if (lr->old_offset == c_invalid_gaia_offset)
                {
                    gaia_offset_t offset = lr->new_offset;

                    ASSERT_INVARIANT(
                        offset != c_invalid_gaia_offset, "An unexpected invalid object offset was found in the log record!");

                    db_object_t* db_object = offset_to_ptr(offset);

                    if (db_object->type == type)
                    {
                        return db_object->id;
                    }
                }
            }
        }

        return std::nullopt;
    };

    return augmented_id_generator;
}

std::shared_ptr<gaia::common::iterators::generator_t<gaia_id_t>>
client_t::get_id_generator_for_type(gaia_type_t type)
{
    int stream_socket = get_id_cursor_socket_for_type(type);
    auto cleanup_stream_socket = make_scope_guard([&]() {
        close_fd(stream_socket);
    });

    auto id_generator = get_stream_generator_for_socket<gaia_id_t>(stream_socket);
    cleanup_stream_socket.dismiss();

    // We need to augment the server-based id generator with a local generator
    // that will also return the elements that have been added by the client
    // in the current transaction, which the server does not yet know about.
    auto augmented_id_generator = augment_id_generator_for_type(type, id_generator);
    return std::make_shared<gaia::common::iterators::generator_t<gaia_id_t>>(augmented_id_generator);
}

static void build_client_request(
    FlatBufferBuilder& builder,
    session_event_t event)
{
    flatbuffers::Offset<client_request_t> client_request;
    client_request = Createclient_request_t(builder, event);
    auto message = Createmessage_t(builder, any_message_t::request, client_request.Union());
    builder.Finish(message);
}

// This function must be called before establishing a new session. It ensures
// that if the server restarts or is reset, no session will start with a stale
// data mapping or locator fd.
void client_t::clear_shared_memory()
{
    // This is intended to be called before a session is established.
    verify_no_session();

    // We closed our original fds for these data segments, so we only need to unmap them.
    data_mapping_t::close(s_data_mappings);
    s_log.close();

    // If the server has already closed its fd for the locator segment
    // (and there are no other clients), this will release it.
    close_fd(s_fd_locators);
}

void client_t::txn_cleanup()
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

int client_t::get_session_socket(const std::string& socket_name)
{
    // Unlike the session socket on the server, this socket must be blocking,
    // because we don't read within a multiplexing poll loop.
    int session_socket = ::socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (session_socket == -1)
    {
        throw_system_error("Socket creation failed!");
    }

    auto cleanup_session_socket = make_scope_guard([&]() {
        close_fd(session_socket);
    });

    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;

    // The socket name (minus its null terminator) needs to fit into the space
    // in the server address structure after the prefix null byte.
    ASSERT_INVARIANT(socket_name.size() <= sizeof(server_addr.sun_path) - 1, "Socket name '" + socket_name + "' is too long!");

    // We prepend a null byte to the socket name so the address is in the
    // (Linux-exclusive) "abstract namespace", i.e., not bound to the
    // filesystem.
    ::strncpy(&server_addr.sun_path[1], socket_name.c_str(), sizeof(server_addr.sun_path) - 1);

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
// process's first call to create_session(), because they are stored globally
// rather than per-session. (The connected socket is stored per-session.)
//
// REVIEW: There is currently no way for the client to be asynchronously notified
// when the server closes the session (e.g., if the server process shuts down).
// Throwing an asynchronous exception on the session thread may not be possible,
// and would be difficult to handle properly even if it were possible.
// In any case, send_msg_with_fds()/recv_msg_with_fds() already throw a
// peer_disconnected exception when the other end of the socket is closed.
void client_t::begin_session(config::session_options_t session_options)
{
    // Fail if a session already exists on this thread.
    verify_no_session();

    // Clean up possible stale state from a server crash or reset.
    clear_shared_memory();

    // Validate shared memory mapping definitions and assert that mappings are not made yet.
    data_mapping_t::validate(s_data_mappings, std::size(s_data_mappings));
    for (auto data_mapping : s_data_mappings)
    {
        ASSERT_INVARIANT(!data_mapping.is_set(), "Segment is already mapped!");
    }

    ASSERT_INVARIANT(!s_log.is_set(), "Log segment is already mapped!");

    s_session_options = session_options;

    // Connect to the server's well-known socket name, and ask it
    // for the data and locator shared memory segment fds.
    s_session_socket = get_session_socket(s_session_options.db_instance_name);

    auto cleanup_session_socket = make_scope_guard([&]() {
        close_fd(s_session_socket);
    });

    // Send the server the connection request.
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::CONNECT);

    client_messenger_t client_messenger;
    client_messenger.send_and_receive(s_session_socket, nullptr, 0, builder, 4);

    // Set up scope guards for the fds.
    // The locators fd needs to be kept around, so its scope guard will be dismissed at the end of this scope.
    // The other fds are not needed, so they'll get their own scope guard to clean them up.
    int fd_locators = client_messenger.received_fd(static_cast<size_t>(data_mapping_t::index_t::locators));
    auto cleanup_fd_locators = make_scope_guard([&]() {
        close_fd(fd_locators);
    });
    auto cleanup_fd_others = make_scope_guard([&]() {
        for (auto data_mapping : s_data_mappings)
        {
            if (data_mapping.mapping_index != data_mapping_t::index_t::locators)
            {
                int fd = client_messenger.received_fd(static_cast<size_t>(data_mapping.mapping_index));
                close_fd(fd);
            }
        }
    });

    session_event_t event = client_messenger.server_reply()->event();
    ASSERT_INVARIANT(event == session_event_t::CONNECT, c_message_unexpected_event_received);

    // Set up the shared-memory mappings.
    // The locators mapping will be performed manually, so skip its information in the mapping table.
    size_t index_fd = 0;
    for (auto data_mapping : s_data_mappings)
    {
        if (data_mapping.mapping_index != data_mapping_t::index_t::locators)
        {
            data_mapping.open(client_messenger.received_fd(index_fd));
        }
        ++index_fd;
    }

    // Set up the private locator segment fd.
    s_fd_locators = fd_locators;

    init_memory_manager();

    cleanup_fd_locators.dismiss();
    cleanup_session_socket.dismiss();
}

void client_t::end_session()
{
    // This will gracefully shut down the server-side session thread
    // and all other threads that session thread owns.
    close_fd(s_session_socket);
}

void client_t::begin_transaction()
{
    verify_session_active();
    verify_no_txn();

    // Map a private COW view of the locator shared memory segment.
    ASSERT_PRECONDITION(!s_private_locators.is_set(), "Locators segment is already mapped!");
    bool manage_fd = false;
    bool is_shared = false;
    s_private_locators.open(s_fd_locators, manage_fd, is_shared);
    auto cleanup_private_locators = make_scope_guard([&]() {
        s_private_locators.close();
    });

    // Send a TXN_BEGIN request to the server and receive a new txn ID,
    // the fd of a new txn log, and txn log fds for all committed txns within
    // the snapshot window.
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::BEGIN_TXN);

    client_messenger_t client_messenger;
    client_messenger.send_and_receive(s_session_socket, nullptr, 0, builder, 1);
    int log_fd = client_messenger.received_fd(client_messenger_t::c_index_txn_log_fd);
    // We can unconditionally close the log fd, because the memory mapping owns
    // an implicit reference to the memfd object.
    auto cleanup_log_fd = make_scope_guard([&]() {
        close_fd(log_fd);
    });

    // Extract the transaction id and cache it; it needs to be reset for the next transaction.
    const transaction_info_t* txn_info = client_messenger.server_reply()->data_as_transaction_info();
    s_txn_id = txn_info->transaction_id();
    ASSERT_INVARIANT(
        s_txn_id != c_invalid_gaia_txn_id,
        "Begin timestamp should not be invalid!");

    // Apply all txn logs received from the server to our snapshot, in order.
    size_t fds_remaining_count = txn_info->log_fds_to_apply_count();
    while (fds_remaining_count > 0)
    {
        client_messenger.receive_server_reply();

        // Apply log fds as we receive them, to avoid having to buffer all of them.
        for (size_t i = 0; i < client_messenger.count_received_fds(); ++i)
        {
            int txn_log_fd = client_messenger.received_fd(i);
            auto cleanup_txn_log_fd = make_scope_guard([&]() { close_fd(txn_log_fd); });
            apply_txn_log(txn_log_fd);
        }

        fds_remaining_count -= client_messenger.count_received_fds();
    }

    // Map the txn log fd we received from the server, for read/write access.
    bool read_only = false;
    s_log.open(log_fd, read_only);

    cleanup_private_locators.dismiss();
}

void client_t::apply_txn_log(int log_fd)
{
    ASSERT_PRECONDITION(s_private_locators.is_set(), "Locators segment must be mapped!");

    mapped_log_t txn_log;
    txn_log.open(log_fd);

    apply_log_to_locators(s_private_locators.data(), txn_log.data());
}

void client_t::rollback_transaction()
{
    verify_txn_active();

    // Clean up all transaction-local session state.
    auto cleanup = make_scope_guard(txn_cleanup);

    // TODO: For now, we do no rollback because the current logic will attempt to
    // deallocate the memory of the new objects manually in deallocate_txn_log().
    // This just means that for now the memory used by rolled back transactions
    // will take longer to be marked as freed.
    // rollback_chunk_manager_allocations();

    // We need to close our log mapping so the server can seal and truncate it.
    s_log.close();

    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::ROLLBACK_TXN);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());
}

// This method returns void on a commit decision and throws on an abort decision.
// It sends a message to the server containing the fd of this txn's log segment and
// will block waiting for a reply from the server.
void client_t::commit_transaction()
{
    verify_txn_active();
    ASSERT_PRECONDITION(s_log.is_set(), "Transaction log must be mapped!");

    // This optimization to treat committing a read-only txn as a rollback
    // allows us to avoid any special cases in the server for empty txn logs.
    if (s_log.data()->record_count == 0)
    {
        rollback_transaction();
        return;
    }

    // Clean up all transaction-local session state when we exit.
    auto cleanup = make_scope_guard(txn_cleanup);

    // We need to close our log mapping so the server can seal and truncate it.
    s_log.close();

    // Send the server the commit message.
    FlatBufferBuilder builder;
    build_client_request(builder, session_event_t::COMMIT_TXN);

    client_messenger_t client_messenger;
    client_messenger.send_and_receive(s_session_socket, nullptr, 0, builder);

    // Extract the commit decision from the server's reply and return it.
    session_event_t event = client_messenger.server_reply()->event();
    ASSERT_INVARIANT(
        event == session_event_t::DECIDE_TXN_COMMIT
            || event == session_event_t::DECIDE_TXN_ABORT
            || event == session_event_t::DECIDE_TXN_ROLLBACK_UNIQUE,
        c_message_unexpected_event_received);

    const transaction_info_t* txn_info = client_messenger.server_reply()->data_as_transaction_info();
    ASSERT_INVARIANT(
        txn_info->transaction_id() == s_txn_id
            || event == session_event_t::DECIDE_TXN_ROLLBACK_UNIQUE,
        "Unexpected transaction id!");

    // Execute trigger only if rules engine is initialized.
    if (s_txn_commit_trigger
        && event == session_event_t::DECIDE_TXN_COMMIT
        && s_events.size() > 0)
    {
        s_txn_commit_trigger(s_events);
    }

    // Throw an exception on server-side abort.
    // REVIEW: We could include the gaia_ids of conflicting objects in
    // transaction_update_conflict
    // (https://gaiaplatform.atlassian.net/browse/GAIAPLAT-292).
    if (event == session_event_t::DECIDE_TXN_ABORT)
    {
        // TODO: For now, we do no rollback because the current logic will attempt to
        // deallocate the memory of the new objects manually in deallocate_txn_log().
        // This just means that for now the memory used by rolled back transactions
        // will take longer to be marked as freed.
        // rollback_chunk_manager_allocations();

        throw transaction_update_conflict();
    }
    // Improving the communication of such errors to the client is tracked by:
    // https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1232
    else if (event == session_event_t::DECIDE_TXN_ROLLBACK_UNIQUE)
    {
        throw index::unique_constraint_violation();
    }

    // TODO: The commit of chunk managers should be done by the server.
    // This requires the client to communicate the offsets of the chunks
    // to the server at the time it requests the commit of the transaction.
    // The call here is just a reminder that this work needs to be done somewhere.
    // commit_chunk_manager_allocations();
}

void client_t::init_memory_manager()
{
    s_memory_manager.load(
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
}

address_offset_t client_t::allocate_object(
    gaia_locator_t locator,
    size_t size)
{
    address_offset_t object_offset = s_chunk_manager.allocate(size + c_db_object_header_size);
    if (object_offset == c_invalid_address_offset)
    {
        // We ran out of memory in the current chunk. Allocate a new one!
        address_offset_t chunk_address_offset = s_memory_manager.allocate_chunk();
        if (chunk_address_offset == c_invalid_address_offset)
        {
            throw memory_allocation_error("Memory manager ran out of memory during call to allocate_chunk().");
        }

        // Keep track of the exhausted chunk manager.
        s_previous_chunk_managers.push_back(s_chunk_manager);

        s_chunk_manager.initialize(
            reinterpret_cast<uint8_t*>(s_shared_data.data()->objects),
            chunk_address_offset);

        // Allocate from new chunk.
        object_offset = s_chunk_manager.allocate(size + c_db_object_header_size);
    }

    ASSERT_POSTCONDITION(object_offset != c_invalid_address_offset, "Chunk manager allocation was not expected to fail!");

    // Update locator array to point to the new offset.
    update_locator(locator, object_offset);

    return object_offset;
}

void client_t::commit_chunk_manager_allocations()
{
    // We commit the chunk managers in the order in which they were allocated.
    //
    // NOTE: when the server will do this work, we will no longer need to make these calls in the client,
    // but this is required until that point, to enable rollback to work properly.
    for (auto& current_chunk_manager : s_previous_chunk_managers)
    {
        current_chunk_manager.commit();
    }

    // We're done with the previous chunk managers.
    s_previous_chunk_managers.clear();

    s_chunk_manager.commit();

    ASSERT_POSTCONDITION(
        s_previous_chunk_managers.empty(),
        "List of previous chunk managers was not emptied by the end of commit!");
}

void client_t::rollback_chunk_manager_allocations()
{
    // We rollback the chunk managers starting with the last one used.
    // At the end of this method, we should be left with one chunk manager
    // and an empty list of previous chunk managers.
    s_chunk_manager.rollback();

    while (!s_previous_chunk_managers.empty())
    {
        s_memory_manager.deallocate_chunk(s_chunk_manager.get_start_memory_offset());

        s_chunk_manager = s_previous_chunk_managers[s_previous_chunk_managers.size() - 1];
        s_previous_chunk_managers.pop_back();

        s_chunk_manager.rollback();
    }

    ASSERT_POSTCONDITION(
        s_previous_chunk_managers.empty(),
        "List of previous chunk managers was not emptied by the end of rollback!");
}
