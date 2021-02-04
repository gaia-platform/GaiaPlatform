/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "se_server.hpp"

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
#include <sys/mman.h>

#include "fd_helpers.hpp"
#include "gaia_db_internal.hpp"
#include "generator_iterator.hpp"
#include "messages_generated.h"
#include "mmap_helpers.hpp"
#include "persistent_store_manager.hpp"
#include "retail_assert.hpp"
#include "scope_guard.hpp"
#include "se_hash_map.hpp"
#include "se_helpers.hpp"
#include "se_object.hpp"
#include "se_object_helpers.hpp"
#include "se_shared_data.hpp"
#include "se_types.hpp"
#include "socket_helpers.hpp"
#include "system_error.hpp"

using namespace std;

using namespace gaia::db;
using namespace gaia::db::messages;
using namespace gaia::common::iterators;
using namespace gaia::common::scope_guard;

// This assignment is non-atomic since there seems to be no reason to expect concurrent invocations.
void server::register_object_deallocator(std::function<void(gaia_offset_t)> deallocator_fn)
{
    s_object_deallocator_fn = deallocator_fn;
}

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

    int send_fds[] = {s_fd_locators, s_fd_counters, s_fd_data, s_fd_id_index, s_fd_page_alloc_counts};
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

    // Open a stream socket to send all applicable txn log fds to the client.
    auto commit_ts_generator = get_commit_ts_generator_for_begin_ts(s_txn_id);

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

    start_fd_stream_producer(server_socket, commit_ts_generator);

    // Transfer ownership of the server socket to the stream producer thread.
    server_socket_cleanup.dismiss();

    // The client must throw an appropriate exception if txn_begin() returns
    // c_invalid_gaia_txn_id. This can only happen when another beginning or
    // committing txn invalidates all unknown timestamps in its snapshot window
    // or conflict window.
    build_server_reply(builder, session_event_t::BEGIN_TXN, old_state, new_state, s_txn_id);

    send_msg_with_fds(s_session_socket, &client_socket, 1, builder.GetBufferPointer(), builder.GetSize());
}

void server::handle_rollback_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(event == session_event_t::ROLLBACK_TXN, "Unexpected event received!");

    // This message should only be received while a transaction is in progress.
    retail_assert(
        old_state == session_state_t::TXN_IN_PROGRESS && new_state == session_state_t::CONNECTED,
        "Current event is inconsistent with state transition!");

    // Release all txn resources and mark the txn's begin_ts entry as terminated.
    txn_rollback();
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
    s_fd_log = *fds;
    retail_assert(s_fd_log != -1, "Uninitialized fd log!");

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
        throw_system_error("fcntl(F_GET_SEALS) failed");
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

void server::handle_decide_txn(
    int*, size_t, session_event_t event, const void*, session_state_t old_state, session_state_t new_state)
{
    retail_assert(
        event == session_event_t::DECIDE_TXN_COMMIT || event == session_event_t::DECIDE_TXN_ABORT,
        "Unexpected event received!");

    retail_assert(
        old_state == session_state_t::TXN_COMMITTING && new_state == session_state_t::CONNECTED,
        "Current event is inconsistent with state transition!");

    // We need to clear transactional state after the decision has been
    // returned, but don't need to free any resources.
    auto cleanup = make_scope_guard([&]() {
        s_txn_id = c_invalid_gaia_txn_id;
        s_fd_log = -1;
    });

    FlatBufferBuilder builder;
    build_server_reply(builder, event, old_state, new_state, s_txn_id);
    send_msg_with_fds(s_session_socket, nullptr, 0, builder.GetBufferPointer(), builder.GetSize());

    // Update the saved watermark and perform associated maintenance tasks. This
    // will block new transactions on this session thread, but that is a
    // feature, not a bug, since it provides natural backpressure on clients who
    // submit long-running transactions that prevent old versions and logs from
    // being freed. This approach helps keep the system from accumulating more
    // deferred work than it can ever retire, which is a problem with performing
    // all maintenance asynchronously in the background. Allowing this work to
    // delay beginning new transactions but not delay committing the current
    // transaction seems like a good compromise.
    update_apply_watermark(s_txn_id);
}

void server::handle_client_shutdown(
    int*, size_t, session_event_t event, const void*, session_state_t, session_state_t new_state)
{
    retail_assert(
        event == session_event_t::CLIENT_SHUTDOWN,
        "Unexpected event received!");

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
        "Unexpected event received!");

    retail_assert(
        new_state == session_state_t::DISCONNECTED,
        "Current event is inconsistent with state transition!");

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
        throw_system_error("socketpair failed");
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
        "Unexpected event received!");

    // This event never changes session state.
    retail_assert(
        old_state == new_state,
        "Current event is inconsistent with state transition!");

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
        "no allowed state transition from state '"
        + std::string(EnumNamesession_state_t(s_session_state))
        + "' with event '"
        + std::string(EnumNamesession_event_t(event))
        + "'");
}

void server::build_server_reply(
    FlatBufferBuilder& builder,
    session_event_t event,
    session_state_t old_state,
    session_state_t new_state,
    gaia_txn_id_t txn_id)
{
    const auto transaction_info = Createtransaction_info_t(builder, txn_id);
    const auto server_reply = Createserver_reply_t(
        builder, event, old_state, new_state, reply_data_t::transaction_info, transaction_info.Union());
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

    unmap_fd(s_page_alloc_counts, sizeof(*s_page_alloc_counts));
    close_fd(s_fd_page_alloc_counts);
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
        throw_system_error("memfd_create failed");
    }

    s_fd_counters = ::memfd_create(c_sch_mem_counters, MFD_ALLOW_SEALING);
    if (s_fd_counters == -1)
    {
        throw_system_error("memfd_create failed");
    }
    s_fd_data = ::memfd_create(c_sch_mem_data, MFD_ALLOW_SEALING);
    if (s_fd_data == -1)
    {
        throw_system_error("memfd_create failed");
    }
    s_fd_id_index = ::memfd_create(c_sch_mem_id_index, MFD_ALLOW_SEALING);
    if (s_fd_id_index == -1)
    {
        throw_system_error("memfd_create failed");
    }
    s_fd_page_alloc_counts = ::memfd_create(c_sch_mem_page_counters, MFD_ALLOW_SEALING);
    if (s_fd_page_alloc_counts == -1)
    {
        throw_system_error("memfd_create failed");
    }

    truncate_fd(s_fd_locators, sizeof(*s_shared_locators));
    truncate_fd(s_fd_counters, sizeof(*s_counters));
    truncate_fd(s_fd_data, sizeof(*s_data));
    truncate_fd(s_fd_id_index, sizeof(*s_id_index));
    truncate_fd(s_fd_page_alloc_counts, sizeof(*s_page_alloc_counts));

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
    map_fd(s_page_alloc_counts, sizeof(*s_page_alloc_counts), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NORESERVE, s_fd_page_alloc_counts, 0);

    // Register an object deallocator that just frees pages when they're empty.
    register_object_deallocator([](gaia_offset_t offset) {
        std::cerr << "Deallocating object with offset " << offset << std::endl;
        size_t byte_offset = offset * sizeof(uint64_t);
        size_t end_byte_offset = byte_offset + size_from_offset(offset);
        size_t start_page_index = byte_offset / c_page_size_bytes;
        size_t end_page_index = end_byte_offset / c_page_size_bytes;
        size_t page_byte_offset = byte_offset % c_page_size_bytes;
        std::cerr << "Deallocating object with offset " << offset << ", byte offset " << page_byte_offset << " on page " << start_page_index << std::endl;
        // For debugging, poison the deallocated object by setting its size field to the max value.
        // NB: we cannot read the object again after this step!
        auto obj = offset_to_ptr(offset);
        obj->payload_size = se_object_t::c_invalid_payload_size;
        // Decrement the allocation count for each page containing this object.
        for (size_t page_index = start_page_index; page_index <= end_page_index; ++page_index)
        {
            size_t old_count = (*s_page_alloc_counts)[page_index].fetch_sub(1);
            std::cerr << "Decrementing allocation count for page index " << page_index << ", old value was " << old_count << ", allocation offset " << offset << std::endl;
            // Don't free the first page until we relocate the last offset counter.
            if (old_count == 1 && page_index > 0)
            {
                // Actually free the page.
                // MADV_REMOVE is supposed to be equivalent to fallocate(FALLOC_FL_PUNCH_HOLE).
                // REVIEW: is MADV_REMOVE the only flag that will work with a memfd mapping?
                // MADV_FREE seems to be the choice for other allocators, but:
                // https://man7.org/linux/man-pages/man2/madvise.2.html
                // "The MADV_FREE operation can be applied only to private anonymous pages."
                // HACKHACK: since offset counter is on first page, we can't free it.
                // Fix this when we move the offset counter to the "counters" shared
                // memory segment.
                void* page_address = reinterpret_cast<unsigned char*>(s_data->objects) + (page_index * c_page_size_bytes);
                cerr << "Freeing page at address " << page_address << endl;
                if (-1 == ::madvise(page_address, c_page_size_bytes, MADV_REMOVE))
                {
                    throw_system_error("madvise(MADV_REMOVE) failed");
                }
            }
        }
    });

    // Populate shared memory from the persistent log and snapshot.
    recover_db();

    cleanup_memory.dismiss();
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

    cerr << "Caught signal " << ::strsignal(signum) << endl;

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
        throw_system_error("socket creation failed");
    }
    auto socket_cleanup = make_scope_guard([&]() { close_fd(listening_socket); });

    // Initialize the socket address structure.
    sockaddr_un server_addr = {0};
    server_addr.sun_family = AF_UNIX;

    // The socket name (minus its null terminator) needs to fit into the space
    // in the server address structure after the prefix null byte.
    static_assert(sizeof(c_se_server_socket_name) <= sizeof(server_addr.sun_path) - 1);

    // We prepend a null byte to the socket name so the address is in the
    // (Linux-exclusive) "abstract namespace", i.e., not bound to the
    // filesystem.
    ::strncpy(&server_addr.sun_path[1], c_se_server_socket_name, sizeof(server_addr.sun_path) - 1);

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
            retail_assert(iter->joinable(), "Thread must be joinable!");
            iter->join();

            // Move the last element into the current entry.
            *iter = std::move(threads.back());
            threads.pop_back();
        }
        else
        {
            // Throw on all other errors (e.g., if the thread has been detached
            // or joined).
            throw_system_error("pthread_kill(0) failed", error);
        }
    }
}

void server::client_dispatch_handler()
{
    // Register session cleanup handler first, so we can execute it last.
    auto session_cleanup = make_scope_guard([]() {
        for (auto& thread : s_session_threads)
        {
            retail_assert(thread.joinable(), "Thread must be joinable!");
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
                retail_assert(false, "Unexpected fd!");
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
    s_session_shutdown_eventfd = make_eventfd();
    auto owned_threads_cleanup = make_scope_guard([]() {
        // Signal all session-owned threads to terminate.
        signal_eventfd(s_session_shutdown_eventfd);

        // Wait for all session-owned threads to terminate.
        for (auto& thread : s_session_owned_threads)
        {
            retail_assert(thread.joinable(), "Thread must be joinable!");
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
            throw_system_error("epoll_wait failed");
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
                    retail_assert(false, "Unexpected event type!");
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
                retail_assert(false, "Unexpected fd!");
            }
            retail_assert(event != session_event_t::NOP, "Unexpected event type!");
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
            throw_system_error("epoll_wait failed");
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
                consume_eventfd(cancel_eventfd);
                producer_shutdown = true;
            }
            else
            {
                // We don't monitor any other fds.
                retail_assert(false, "Unexpected fd!");
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

// REVIEW: this is copied from stream_producer_handler() only for expediency;
// the epoll loop boilerplate will be factored out in the near future.
void server::fd_stream_producer_handler(
    int stream_socket, int cancel_eventfd, std::function<std::optional<gaia_txn_id_t>()> ts_generator_fn)
{
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

    auto gen_iter = generator_iterator_t<gaia_txn_id_t>(ts_generator_fn);

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

    // The userspace buffer that we use to construct batched ancillary data.
    std::vector<int> batch_buffer;

    // We need to call reserve() rather than the "sized" constructor to avoid changing size().
    batch_buffer.reserve(c_max_fd_count);

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
                    cerr << "fd stream socket error: " << ::strerror(error) << endl;
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
                    while (gen_iter && (batch_buffer.size() < c_max_fd_count))
                    {
                        gaia_txn_id_t next_ts = *gen_iter;
                        retail_assert(
                            is_commit_ts(next_ts) && is_txn_committed(next_ts),
                            "we can only send clients the log fd of a committed txn!");

                        // Since the watermark could advance past the saved
                        // value used in the commit_ts generator, we need to be
                        // sure that we don't send an invalidated and closed log
                        // fd, so we validate and duplicate each log fd using
                        // the safe_fd_from_ts class before sending it to the
                        // client. (We let our callers handle the invalid_log_fd
                        // exception thrown from the safe_fd_from_ts constructor
                        // if the log fd is concurrently invalidated.) We set
                        // auto_close_fd = false in the safe_fd_from_ts
                        // constructor because we need to save the dup fds in
                        // the buffer until we pass them to sendmsg().
                        try
                        {
                            safe_fd_from_ts committed_txn_log_fd(next_ts, false);
                            int local_log_fd = committed_txn_log_fd.get_fd();

                            batch_buffer.push_back(local_log_fd);
                        }
                        catch (const invalid_log_fd&)
                        {
                            // We ignore an invalidated fd since its log has
                            // already been applied to the shared locator view,
                            // so we don't need to send it to the client anyway.
                        }

                        ++gen_iter;
                    }
                    // We need to send any pending data in the buffer, followed by EOF
                    // if we reached end of iteration. We let the client decide when to
                    // close the socket, since their next read may be arbitrarily delayed
                    // (and they may still have pending data).
                    // First send any remaining data in the buffer.
                    if (batch_buffer.size() > 0)
                    {
                        retail_assert(
                            batch_buffer.size() <= c_max_fd_count,
                            "Buffer has more than the maximum allowed number of fds!");

                        for (auto& fd : batch_buffer)
                        {
                            retail_assert(is_fd_valid(fd), "invalid fd!");
                        }

                        // To simplify client state management by allowing the client to
                        // dequeue entries in FIFO order using std::vector.pop_back(),
                        // we reverse the order of entries in the buffer.
                        std::reverse(std::begin(batch_buffer), std::end(batch_buffer));

                        // We only need a 1-byte buffer due to our datagram size convention.
                        uint8_t msg_buf[1] = {0};
                        ssize_t bytes_written = send_msg_with_fds(
                            stream_socket, batch_buffer.data(), batch_buffer.size(), msg_buf, sizeof(msg_buf));

                        if (bytes_written == -1)
                        {
                            // It should never happen that the socket is no longer writable
                            // after we receive EPOLLOUT, since we are the only writer and
                            // the receive buffer is always large enough for a batch.
                            retail_assert(errno != EAGAIN && errno != EWOULDBLOCK, "Unexpected errno value!");
                            // Log the error and break out of the poll loop.
                            cerr << "fd stream socket error: " << ::strerror(errno) << endl;
                            producer_shutdown = true;
                        }
                        else
                        {
                            // We successfully wrote to the socket, so clear the buffer.
                            // (Partial writes are impossible with datagram sockets.)
                            // The standard is somewhat unclear, but apparently clear() will
                            // not change the capacity in any recent implementation of the
                            // standard library (https://cplusplus.github.io/LWG/issue1102).
                            // First we need to close each dup fd in the buffer.
                            for (auto& dup_fd : batch_buffer)
                            {
                                close_fd(dup_fd);
                            }
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
                consume_eventfd(cancel_eventfd);
                producer_shutdown = true;
            }
            else
            {
                // We don't monitor any other fds.
                retail_assert(false, "Unexpected fd!");
            }
        }
    }
}

void server::start_fd_stream_producer(int stream_socket, std::function<std::optional<gaia_txn_id_t>()> ts_generator_fn)
{
    // First reap any owned threads that have terminated (to avoid memory and
    // system resource leaks).
    reap_exited_threads(s_session_owned_threads);
    // Create fd stream producer thread.
    s_session_owned_threads.emplace_back(
        fd_stream_producer_handler, stream_socket, s_session_shutdown_eventfd, ts_generator_fn);
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
    // Scan timestamp entries from the last applied commit_ts forward.
    // Invalidate any unknown entries and validate any undecided txns.
    for (gaia_txn_id_t ts = start_ts; ts < end_ts; ++ts)
    {
        // Fence off any txns that might have allocated a commit_ts before our
        // begin_ts but have not yet registered their txn log under their
        // commit_ts.
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

std::function<std::optional<gaia_txn_id_t>()> server::get_commit_ts_generator_for_begin_ts(gaia_txn_id_t begin_ts)
{
    gaia_txn_id_t ts = s_last_applied_commit_ts_upper_bound;
    return [=]() mutable -> std::optional<int> {
        while (++ts && ts < begin_ts)
        {
            // Snapshot isolation requires that we can only apply txns before this begin_ts to the snapshot.
            retail_assert(ts < begin_ts, "Can't apply commit_ts newer than the begin_ts of this txn!");
            if (is_commit_ts(ts))
            {
                retail_assert(
                    is_txn_decided(ts),
                    "Undecided commit_ts found in snapshot window!");
                if (is_txn_committed(ts))
                {
                    return ts;
                }
            }
        }
        // Signal end of iteration.
        return std::nullopt;
    };
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
    std::bitset<c_txn_status_entry_bits> entry_bits(entry);

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
        std::bitset<c_txn_status_entry_bits> entry_bits(entry);
        cerr << "Timestamp entry for commit_ts entry's begin_ts " << begin_ts << ": " << entry_bits << endl;
        cerr << "Log FD for commit_ts entry: " << get_txn_log_fd(ts) << endl;
    }

    if (is_txn_entry_begin_ts(entry) && is_txn_entry_submitted(entry))
    {
        gaia_txn_id_t commit_ts = get_commit_ts(ts);
        if (commit_ts != c_invalid_gaia_txn_id)
        {
            // We can't recurse here since we'd just bounce back and forth between a
            // txn's begin_ts and commit_ts.
            ts_entry_t entry = s_txn_info[commit_ts];
            std::bitset<c_txn_status_entry_bits> entry_bits(entry);
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
    retail_assert(is_begin_ts(begin_ts), "Not a begin timestamp!");
    ts_entry_t begin_ts_entry = s_txn_info[begin_ts];
    return is_txn_entry_submitted(begin_ts_entry);
}

inline bool server::is_txn_entry_validating(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_validating);
}

inline bool server::is_txn_validating(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), "Not a commit timestamp!");
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
    retail_assert(is_txn_entry_commit_ts(commit_ts_entry), "Not a commit timestamp!");
    return is_txn_entry_decided(commit_ts_entry);
}

inline bool server::is_txn_entry_committed(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_committed);
}

inline bool server::is_txn_committed(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), "Not a commit timestamp!");
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    return is_txn_entry_committed(commit_ts_entry);
}

inline bool server::is_txn_entry_aborted(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_aborted);
}

inline bool server::is_txn_aborted(gaia_txn_id_t commit_ts)
{
    retail_assert(is_commit_ts(commit_ts), "Not a commit timestamp!");
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    return is_txn_entry_aborted(commit_ts_entry);
}

inline bool server::is_txn_entry_active(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_active);
}

inline bool server::is_txn_active(gaia_txn_id_t begin_ts)
{
    retail_assert(begin_ts != c_txn_entry_unknown, "Unknown timestamp!");
    retail_assert(is_begin_ts(begin_ts), "Not a begin timestamp!");
    ts_entry_t ts_entry = s_txn_info[begin_ts];
    return is_txn_entry_active(ts_entry);
}

inline bool server::is_txn_entry_terminated(ts_entry_t ts_entry)
{
    return (get_status_from_entry(ts_entry) == c_txn_status_terminated);
}

inline bool server::is_txn_terminated(gaia_txn_id_t begin_ts)
{
    retail_assert(begin_ts != c_txn_entry_unknown, "Unknown timestamp!");
    retail_assert(is_begin_ts(begin_ts), "Not a begin timestamp!");
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
    retail_assert(is_commit_ts(commit_ts), "Not a commit timestamp!");
    ts_entry_t commit_ts_entry = s_txn_info[commit_ts];
    // The begin_ts is the low 42 bits of the ts entry.
    auto begin_ts = static_cast<gaia_txn_id_t>(commit_ts_entry & c_txn_ts_mask);
    retail_assert(begin_ts != c_invalid_gaia_txn_id, "begin_ts must be valid!");
    return begin_ts;
}

inline gaia_txn_id_t server::get_commit_ts(gaia_txn_id_t begin_ts)
{
    retail_assert(is_begin_ts(begin_ts), "Not a begin timestamp!");
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
    retail_assert(is_commit_ts(commit_ts), "Not a commit timestamp!");
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
    retail_assert(is_commit_ts(commit_ts), "Not a commit timestamp!");
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
    //
    // - Find all committed transactions in conflict window, i.e., between our
    //   begin and commit timestamps, traversing from oldest to newest txns and
    //   testing for conflicts as we go, to allow as much time as possible for
    //   undecided txns to be validated, and for commit timestamps allocated
    //   within our conflict window to be registered. Skip over all unknown
    //   timestamps, invalidated timestamps, active txns, undecided txns, and
    //   aborted txns.
    //
    // - Invalidate all unknown timestamps, so no txns can be submitted within
    //   our conflict window. Any submitted txns which have allocated a commit
    //   timestamp within our conflict window but have not yet registered their
    //   commit timestamp must now retry with a new timestamp. (This allows us
    //   to treat our conflict window as an immutable snapshot of submitted
    //   txns, without needing to acquire any locks.)
    //
    // - Important optimization: find the latest undecided txn which conflicts
    //   with our write set. Any undecided txns later than this don't need to be
    //   validated (but earlier, non-conflicting undecided txns still need to be
    //   validated, since they could conflict with a later undecided txn with a
    //   conflicting write set, which would abort and hence not cause us to
    //   abort).
    //
    // - Recursively validate all undecided txns preceding the last conflicting
    //   undecided txn identified in the preceding step, from oldest to newest.
    //   Note that we cannot recurse indefinitely, since by hypothesis no
    //   undecided txns can precede our begin timestamp (because a new
    //   transaction must validate any undecided transactions with commit
    //   timestamps preceding its begin timestamp before it can proceed).
    //   However, we could duplicate work with other session threads validating
    //   their committing txns. We mitigate this by 1) tracking all txns'
    //   validation status in their commit timestamp entries, and 2) rechecking
    //   validation status for the current txn after scanning each possibly
    //   conflicting txn log (and aborting validation if it has already been
    //   validated).
    //
    // - Test any committed txns validated in the preceding step for conflicts.
    //
    // - If any of these steps finds that our write set conflicts with the write
    //   set of any committed txn, we must abort. Otherwise, we commit. In
    //   either case, we set the decided state in our commit timestamp entry and
    //   return the commit decision to the client.

    // Find commit timestamps of all committed and undecided txns in conflict window.
    std::unordered_set<gaia_txn_id_t> committed_txns;

    // Iterate over all txns in conflict window, and test all committed txns for conflicts.
    for (gaia_txn_id_t ts = get_begin_ts(commit_ts) + 1; ts < commit_ts; ++ts)
    {
        // Invalidate all unknown timestamps. This marks a "fence" after which
        // any submitted txns with commit timestamps in our conflict window must
        // already be registered under their commit_ts (they must retry with a
        // new timestamp otherwise).
        if (invalidate_unknown_ts(ts))
        {
            continue;
        }

        if (is_commit_ts(ts) && is_txn_committed(ts))
        {
            // Remember each committed txn commit_ts so we don't test it again.
            committed_txns.insert(ts);
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
                    "A txn log fd can only be invalidated if its commit_ts has been validated!");
                if (invalidated_commit_ts == ts)
                {
                    retail_assert(
                        is_txn_decided(commit_ts),
                        "The txn being tested can only have its log fd invalidated if the validating txn was validated!");
                }
                // If either log fd was invalidated, then the validating txn
                // must have been validated, so we can return the decision
                // immediately.
                return is_txn_committed(commit_ts);
            }
        }
        // Check if another thread has already validated this txn.
        if (is_txn_decided(commit_ts))
        {
            return is_txn_committed(commit_ts);
        }
    }

    // Find the latest undecided conflicting txn, to determine when we can stop
    // validating undecided txns.
    gaia_txn_id_t last_undecided_conflicting_txn_commit_ts = c_invalid_gaia_txn_id;
    // Iterate backwards over conflict window so we find the most recent txn first.
    for (gaia_txn_id_t ts = commit_ts - 1; ts > get_begin_ts(commit_ts); --ts)
    {
        if (is_commit_ts(ts) && is_txn_validating(ts))
        {
            // We need to use the safe_fd_from_ts wrapper for conflict detection
            // in case either log fd is invalidated by another thread
            // concurrently advancing the watermark. If either log fd is
            // invalidated, it must be that another thread has validated our
            // txn, so we can exit early.
            try
            {
                safe_fd_from_ts validating_fd(commit_ts);
                safe_fd_from_ts undecided_fd(ts);

                if (txn_logs_conflict(validating_fd.get_fd(), undecided_fd.get_fd()))
                {
                    last_undecided_conflicting_txn_commit_ts = ts;
                    break;
                }
            }
            catch (const invalid_log_fd& e)
            {
                // invalid_log_fd can only be thrown if the
                // log fd of the commit_ts it references has been invalidated, which can only happen if the
                // watermark has advanced past the commit_ts.
                // The watermark cannot advance past our begin_ts unless our txn
                // has already been validated, so if either of the fds we are
                // testing for conflicts is invalidated, it must mean that our
                // txn has already been validated. Since our commit_ts always
                // follows the commit_ts of the undecided txn we are testing for
                // conflicts, and the watermark always advances in order, it
                // cannot be the case that this txn's log fd has not been
                // invalidated while our txn's log fd has been invalidated.
                gaia_txn_id_t invalidated_commit_ts = e.get_ts();
                retail_assert(
                    is_txn_decided(invalidated_commit_ts),
                    "A txn log fd can only be invalidated if its commit_ts has been validated!");
                if (invalidated_commit_ts == commit_ts)
                {
                    retail_assert(
                        get_txn_log_fd(ts) == -1,
                        "If the validating txn's log fd has been invalidated, then the watermark must have advanced past the commit_ts of the txn being tested, and invalidated its log fd!");
                }
                else
                {
                    retail_assert(
                        invalidated_commit_ts == ts,
                        "The commit_ts whose log fd was invalidated must belong to either the validating txn or the txn being tested!");
                    retail_assert(
                        is_txn_decided(commit_ts),
                        "The txn being tested can only have its log fd invalidated if the validating txn was validated!");
                }
                // If either log fd was invalidated, then the validating txn
                // must have been validated, so we can return the decision
                // immediately.
                return is_txn_committed(commit_ts);
            }
        }
        // Check if another thread has already validated this txn.
        if (is_txn_decided(commit_ts))
        {
            return is_txn_committed(commit_ts);
        }
    }

    // Before we validate undecided txns, check for any committed txns that were
    // previously undecided and check them for conflicts, to give any undecided
    // txns a chance for validation, and to allow us to terminate early if we
    // find a conflict, without validating any more undecided txns.
    for (gaia_txn_id_t ts = get_begin_ts(commit_ts) + 1; ts < commit_ts; ++ts)
    {
        if (is_commit_ts(ts) && is_txn_committed(ts))
        {
            // Only test for conflicts if we didn't already test it.
            const auto [iter, is_new_ts] = committed_txns.insert(ts);
            if (is_new_ts)
            {
                // We need to use the safe_fd_from_ts wrapper for conflict detection
                // in case either log fd is invalidated by another thread
                // concurrently advancing the watermark. If either log fd is
                // invalidated, it must be that another thread has validated our
                // txn, so we can exit early.
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
                        "A txn log fd can only be invalidated if its commit_ts has been validated!");
                    if (invalidated_commit_ts == commit_ts)
                    {
                        retail_assert(
                            get_txn_log_fd(ts) == -1,
                            "If the validating txn's log fd has been invalidated, then the watermark must have advanced past the commit_ts of the txn being tested, and invalidated its log fd!");
                    }
                    else
                    {
                        retail_assert(
                            invalidated_commit_ts == ts,
                            "The commit_ts whose log fd was invalidated must belong to either the validating txn or the txn being tested!");
                        retail_assert(
                            is_txn_decided(commit_ts),
                            "The txn being tested can only have its log fd invalidated if the validating txn was validated!");
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

    // If there are no potentially conflicting undecided txns, we are done.
    if (last_undecided_conflicting_txn_commit_ts == c_invalid_gaia_txn_id)
    {
        return true;
    }

    // Validate all undecided txns preceding the last conflicting undecided txn,
    // from oldest to newest. If any validated txn commits, test it immediately
    // for conflicts.
    for (gaia_txn_id_t ts = get_begin_ts(commit_ts) + 1; ts < last_undecided_conflicting_txn_commit_ts; ++ts)
    {
        if (is_commit_ts(ts) && is_txn_validating(ts))
        {
            // By hypothesis, there are no undecided txns with commit timestamps
            // preceding the committing txn's begin timestamp.
            retail_assert(
                ts > s_txn_id,
                "A transaction with commit timestamp preceding this transaction's begin timestamp is undecided!");

            // Recursively validate the current undecided txn.
            bool committed = validate_txn(ts);

            // Update the current txn's decided status.
            update_txn_decision(ts, committed);

            // If the just-validated txn committed, then test it for conflicts.
            if (committed)
            {
                // We need to use the safe_fd_from_ts wrapper for conflict detection
                // in case either log fd is invalidated by another thread
                // concurrently advancing the watermark. If either log fd is
                // invalidated, it must be that another thread has validated our
                // txn, so we can exit early.
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
                    // invalid_log_fd can only be thrown if the
                    // log fd of the commit_ts it references has been invalidated, which can only happen if the
                    // watermark has advanced past the commit_ts.
                    // The watermark cannot advance past our begin_ts unless our txn
                    // has already been validated, so if either of the fds we are
                    // testing for conflicts is invalidated, it must mean that our
                    // txn has already been validated. Since our commit_ts always
                    // follows the commit_ts of the undecided txn we are testing for
                    // conflicts, and the watermark always advances in order, it
                    // cannot be the case that this txn's log fd has not been
                    // invalidated while our txn's log fd has been invalidated.
                    gaia_txn_id_t invalidated_commit_ts = e.get_ts();
                    retail_assert(
                        is_txn_decided(invalidated_commit_ts),
                        "A txn log fd can only be invalidated if its commit_ts has been validated!");
                    if (invalidated_commit_ts == commit_ts)
                    {
                        retail_assert(
                            get_txn_log_fd(ts) == -1,
                            "If the validating txn's log fd has been invalidated, then the watermark must have advanced past the commit_ts of the txn being tested, and invalidated its log fd!");
                    }
                    else
                    {
                        retail_assert(
                            invalidated_commit_ts == ts,
                            "The commit_ts whose log fd was invalidated must belong to either the validating txn or the txn being tested!");
                        retail_assert(
                            is_txn_decided(commit_ts),
                            "The txn being tested can only have its log fd invalidated if the validating txn was validated!");
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

    // At this point, there are no undecided txns that could possibly conflict
    // with this txn, and all committed txns have been tested for conflicts, so
    // we can commit.
    return true;
}

// This advances the given global timestamp counter to the given timestamp
// value, unless it has already advanced beyond the given value due to a
// concurrent update.
//
// NB: we use compare_exchange_weak() for the global update since we need to
// retry anyway on concurrent updates, so tolerating spurious failures
// requires no additional logic.
bool server::advance_watermark_ts(std::atomic<gaia_txn_id_t>& watermark, gaia_txn_id_t ts)
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

void server::apply_txn_redo_log_from_ts(gaia_txn_id_t commit_ts)
{
    retail_assert(
        is_commit_ts(commit_ts) && is_txn_committed(commit_ts),
        "apply_txn_log_from_ts() must be called on the commit_ts of a committed txn!");

    // We need to use the safe_fd_from_ts wrapper in case our log fd is
    // invalidated by another thread concurrently advancing the watermark. We
    // let our callers handle the invalid_log_fd exception thrown from the
    // safe_fd_from_ts constructor if the log fd is concurrently invalidated.
    safe_fd_from_ts committed_txn_log_fd(commit_ts);
    int local_log_fd = committed_txn_log_fd.get_fd();

    txn_log_t* txn_log;
    map_fd(txn_log, get_fd_size(local_log_fd), PROT_READ, MAP_PRIVATE, local_log_fd, 0);
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
}

void server::gc_txn_undo_log(int log_fd, bool committed)
{
    txn_log_t* txn_log;
    map_fd(txn_log, get_fd_size(log_fd), PROT_READ, MAP_PRIVATE, log_fd, 0);
    // Ensure the fd is unmapped when we exit this scope.
    auto cleanup_log_mapping = make_scope_guard([&]() {
        unmap_fd(txn_log, txn_log->size());
    });

    cerr << "Freeing log with fd " << log_fd << " on " << (committed ? "COMMIT" : "ABORT") << ":\n"
         << *txn_log << endl;

    for (size_t i = 0; i < txn_log->record_count; ++i)
    {
        // For committed txns, free each undo version (i.e., the version
        // superseded by an update or delete operation), using the registered
        // object deallocator (if it exists), since the redo versions may still
        // be visible but the undo versions cannot be. For aborted txns, free
        // only the redo versions (since the undo versions may still be
        // visible).
        // NB: we can't safely free the redo versions and txn logs of aborted
        // txns in the decide handler, because concurrently validating txns
        // might be testing the aborted txn for conflicts while they still think
        // it is undecided. The only safe place to free the redo versions and
        // txn log of an aborted txn is after it falls behind the watermark,
        // since at that point it cannot be in the conflict window of any
        // committing txn.
        gaia_offset_t offset_to_free = committed
            ? txn_log->log_records[i].old_offset
            : txn_log->log_records[i].new_offset;
        if (offset_to_free && s_object_deallocator_fn)
        {
            s_object_deallocator_fn(offset_to_free);
        }
    }
}

// This method is called by an active txn when it is terminated or validated. It
// finds the current oldest terminated or validated txn and applies the redo
// logs of all committed txns preceding that txn's begin_ts to the shared view.
// Then it advances a saved "watermark" variable to the commit_ts of the latest
// txn it applied to the shared locator view, so we know which logs to apply
// during snapshot reconstruction. Finally, it frees object versions in the undo
// logs of committed txns that were applied to the shared locator view, and then
// deallocates the logs themselves.
//
// REVIEW: I don't *think* there are any races in this function from reading the
// same timestamp entry repeatedly with the is_txn_*(ts) functions, but to be on
// the safe side we could just read each ts entry once and use the
// is_txn_entry_*(entry) functions instead.
//
// REVIEW: This algorithm uses a CAS on every step of the traversal, which is
// very expensive and might cause a lot of cache-coherency traffic on many-core
// machines. We do abort the traversal when concurrency is detected via CAS
// failure, but this will help only if uncontended CAS is cheaper than contended
// CAS (which may not be true on all architectures). I think it should be
// possible to design an algorithm that only does CAS at the very end of the
// traversal, when we try to install the new watermark. Such an algorithm might
// require two watermarks: an "apply" watermark and an "invalidate" watermark,
// where "apply" and "invalidate" operations would be executed after their
// respective watermarks had advanced (right now "apply" is done before
// advancing the watermark and "invalidate" is done after).
//
// TODO: deallocate physical pages backing s_txn_info for addresses preceding
// the watermark (via madvise(MADV_FREE)).

void server::update_apply_watermark(gaia_txn_id_t begin_ts)
{
    // The ts supplied must be a begin_ts.
    retail_assert(is_begin_ts(begin_ts), "update_apply_watermark() called with a non-begin_ts!");

    // First get a fuzzy snapshot of the timestamp counter (we don't know yet if
    // this is a begin_ts or commit_ts).
    gaia_txn_id_t last_allocated_ts = get_last_txn_id();
    // Now get a snapshot of the watermark, so we can scan forward from there.
    gaia_txn_id_t last_applied_commit_ts_upper_bound = s_last_applied_commit_ts_upper_bound;

    // Scan from the saved watermark to the last known timestamp, to find the
    // oldest active txn (if any) after begin_ts and the newest committed txn
    // (if any) preceding the oldest active txn if it exists, or before the last
    // known timestamp otherwise.

    for (gaia_txn_id_t ts = last_applied_commit_ts_upper_bound + 1; ts <= last_allocated_ts; ++ts)
    {
        // We need to invalidate unknown entries as we go along, so that we
        // don't miss any active begin_ts or committed commit_ts entries.
        invalidate_unknown_ts(ts);

        // If this is a commit_ts, we cannot advance the watermark unless it's
        // decided. In that case, we apply its txn log (if it's committed)
        // before advancing the watermark, and invalidate and close its log fd
        // after advancing the watermark.
        if (is_commit_ts(ts))
        {
            if (is_txn_validating(ts))
            {
                break;
            }

            if (is_txn_committed(ts))
            {
                try
                {
                    apply_txn_redo_log_from_ts(ts);
                }
                catch (const invalid_log_fd&)
                {
                    // If apply_txn_log_from_ts() throws invalid_log_fd, then
                    // the log fd has already been invalidated and (presumably)
                    // closed. Since the log fd can only be invalidated after
                    // it's been applied and the last_applied watermark has been
                    // updated, we know the thread which invalidated it must
                    // have already applied it and advanced the watermark. Since
                    // another thread is advancing the watermark, we abort
                    // advancing it further.
                    retail_assert(
                        s_last_applied_commit_ts_upper_bound > last_applied_commit_ts_upper_bound,
                        "The watermark must have advanced if a commit_ts had its log fd invalidated!");
                    break;
                }
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

        if (!advance_watermark_ts(s_last_applied_commit_ts_upper_bound, ts))
        {
            // If another thread has already advanced the watermark ahead of
            // this ts, we abort advancing it further.
            retail_assert(
                s_last_applied_commit_ts_upper_bound > last_applied_commit_ts_upper_bound,
                "The watermark must have advanced if advance_watermark_ts failed!");
            break;
        }

        if (is_commit_ts(ts))
        {
            retail_assert(is_txn_decided(ts), "The watermark should not be advanced to an undecided commit_ts!");

            // We always invalidate and close the log fd of a decided
            // commit_ts after we have advanced the watermark. Since we
            // successfully advanced the watermark to this commit_ts, no
            // other thread could have previously advanced it, and since
            // invalidation is always done after advancing the watermark, no
            // other thread could have invalidated the log fd.

            int log_fd = get_txn_log_fd(ts);
            retail_assert(log_fd != -1, "log fd cannot be invalidated if we advanced the watermark first!");
            retail_assert(is_fd_valid(log_fd), "log fd cannot be closed if we advanced the watermark first!");
            // Invalidate the log fd, GC the undo versions in the log, and close the fd.
            bool has_invalidated_fd = invalidate_txn_log_fd(ts);
            retail_assert(has_invalidated_fd, "Invalidation must succeed if we were the first thread to advance the watermark to this commit_ts!");
            // If the txn committed, we deallocate only undo versions, since the
            // redo versions may still be visible after the txn has fallen
            // behind the watermark. If the txn aborted, then we deallocate only
            // redo versions, since the undo versions may still be visible. Note
            // that we could deallocate intermediate versions (i.e., those
            // superseded within the same txn) immediately, but we do it here
            // for simplicity.
            gc_txn_undo_log(log_fd, is_txn_committed(ts));
            close_fd(log_fd);
        }
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

    // Set our txn status to TXN_TERMINATED.
    set_active_txn_terminated(s_txn_id);

    // Update the saved watermark and perform associated maintenance tasks.
    update_apply_watermark(s_txn_id);

    // This session now has no active txn.
    s_txn_id = c_invalid_gaia_txn_id;
}

// Before this method is called, we have already received the log fd from the client
// and mmapped it.
// This method returns true for a commit decision and false for an abort decision.
bool server::txn_commit()
{
    retail_assert(s_fd_log != -1, "Uninitialized fd log!");

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
    retail_assert(s_fd_log != -1, "Uninitialized fd log!");

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

        init_shared_memory();
        init_txn_info();
        s_last_applied_commit_ts_upper_bound = c_invalid_gaia_txn_id;

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
