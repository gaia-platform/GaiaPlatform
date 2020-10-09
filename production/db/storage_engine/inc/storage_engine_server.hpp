/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <csignal>
#include <thread>
#include <shared_mutex>

#include "scope_guard.hpp"
#include "retail_assert.hpp"
#include "system_error.hpp"
#include "mmap_helpers.hpp"
#include "socket_helpers.hpp"
#include "generator_iterator.hpp"

#include "storage_engine.hpp"
#include "gaia_se_object.hpp"
#include "messages_generated.h"
#include "persistent_store_manager.hpp"
#include "gaia_se_object.hpp"
#include "gaia_db_internal.hpp"

namespace gaia {
namespace db {

using namespace gaia::common;
using namespace gaia::common::iterators;
using namespace gaia::db::messages;
using namespace flatbuffers;
using namespace scope_guard;

class invalid_session_transition : public gaia_exception {
public:
    invalid_session_transition(const string& message) : gaia_exception(message) {}
};

class server : private se_base {
    friend class persistent_store_manager;

public:
    static void run();

private:
    // from https://www.man7.org/linux/man-pages/man2/eventfd.2.html
1    static constexpr uint64_t MAX_SEMAPHORE_COUNT = std::numeric_limits<uint64_t>::max() - 1;
    // This is arbitrary but seems like a reasonable starting point (pending benchmarks).
    static constexpr size_t STREAM_BATCH_SIZE = 1 << 10;
    static int s_server_shutdown_eventfd;
    static int s_connect_socket;
    static std::shared_mutex s_locators_lock;
    static int s_fd_data;
    static locators* s_shared_locators;
    static std::unique_ptr<persistent_store_manager> rdb;
    thread_local static session_state_t s_session_state;
    thread_local static bool s_session_shutdown;
    thread_local static int s_session_shutdown_eventfd;
    thread_local static std::vector<std::thread> s_session_owned_threads;

    static int s_fd_locators;
    static data* s_data;

    // Inherited from se_base:
    // thread_local static log *s_log;
    // thread_local static int s_session_socket;
    // thread_local static gaia_txn_id_t s_txn_id;

    // function pointer type that executes side effects of a state transition
    // REVIEW: replace void* with std::any?
    typedef void (*transition_handler_fn)(int* fds, size_t fd_count, session_event_t event,
        const void* event_data, session_state_t old_state, session_state_t new_state);
    static void handle_connect(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_begin_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_rollback_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_commit_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_decide_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_client_shutdown(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_server_shutdown(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_request_stream(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);

    struct transition_t {
        session_state_t new_state;
        transition_handler_fn handler;
    };

    struct valid_transition_t {
        session_state_t state;
        session_event_t event;
        transition_t transition;
    };

    // DISCONNECTED (client has connected to server listening socket, authenticated, and requested session)
    // -> CONNECTED
    // CONNECTED (client datagram socket has been allocated, client thread has been started,
    // server has replied to client from listening socket with its session socket,
    // client has connected to server from that socket, server has replied with fds
    // for data/locator segments)
    // -> TXN_IN_PROGRESS (client called begin_transaction)
    // -> DISCONNECTING (server half-closed session socket)
    // -> DISCONNECTED (client closed or half-closed session socket)
    // TXN_IN_PROGRESS (client sent begin_transaction message to server, server replied)
    // -> TXN_COMMITTING (client called commit_transaction)
    // -> CONNECTED (client rolled back transaction)
    // TXN_COMMITTING (server decided to commit or abort transaction)
    // -> CONNECTED

    static constexpr valid_transition_t s_valid_transitions[] = {
        {session_state_t::DISCONNECTED, session_event_t::CONNECT, {session_state_t::CONNECTED, handle_connect}},
        {session_state_t::ANY, session_event_t::CLIENT_SHUTDOWN, {session_state_t::DISCONNECTED, handle_client_shutdown}},
        {session_state_t::CONNECTED, session_event_t::BEGIN_TXN, {session_state_t::TXN_IN_PROGRESS, handle_begin_txn}},
        {session_state_t::TXN_IN_PROGRESS, session_event_t::ROLLBACK_TXN, {session_state_t::CONNECTED, handle_rollback_txn}},
        {session_state_t::TXN_IN_PROGRESS, session_event_t::COMMIT_TXN, {session_state_t::TXN_COMMITTING, handle_commit_txn}},
        {session_state_t::TXN_COMMITTING, session_event_t::DECIDE_TXN_COMMIT, {session_state_t::CONNECTED, handle_decide_txn}},
        {session_state_t::TXN_COMMITTING, session_event_t::DECIDE_TXN_ABORT, {session_state_t::CONNECTED, handle_decide_txn}},
        {session_state_t::ANY, session_event_t::SERVER_SHUTDOWN, {session_state_t::DISCONNECTED, handle_server_shutdown}},
        {session_state_t::ANY, session_event_t::REQUEST_STREAM, {session_state_t::ANY, handle_request_stream}},
    };

    static void apply_transition(session_event_t event, const void* event_data, int* fds, size_t fd_count);

    static void build_server_reply(
        FlatBufferBuilder& builder,
        session_event_t event,
        session_state_t old_state,
        session_state_t new_state,
        gaia_txn_id_t txn_id = 0);

    static void clear_shared_memory();

    static void init_shared_memory();

    static void recover_db() {
        // Open RocksDB just once.
        if (!rdb) {
            rdb.reset(new gaia::db::persistent_store_manager());
            rdb->open();
            rdb->recover();
        }
    }

    static sigset_t mask_signals();

    static void signal_handler(sigset_t sigset, int& signum);

    static int get_client_dispatch_fd();

    static bool authenticate_client_socket(const int socket);

    static void client_dispatch_handler();

    static void session_handler(const int session_socket);


    template <typename element_type>
    static void stream_producer_handler(const int stream_socket, const int cancel_eventfd,
        std::function<std::optional<element_type>()> generator_fn);


    template <typename element_type>
    static void start_stream_producer(const int stream_socket,
        std::function<std::optional<element_type>()> generator_fn);

    static std::function<std::optional<gaia_id_t>()>
    get_id_generator_for_type(const gaia_type_t type);

    static bool tx_commit();
};

} // namespace db
} // namespace gaia
