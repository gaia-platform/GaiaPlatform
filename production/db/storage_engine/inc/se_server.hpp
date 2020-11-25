/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <csignal>

#include <functional>
#include <optional>
#include <shared_mutex>
#include <thread>

#include "flatbuffers/flatbuffers.h"

#include "gaia/exception.hpp"
#include "messages_generated.h"
#include "persistent_store_manager.hpp"
#include "se_types.hpp"

namespace gaia
{
namespace db
{

using namespace gaia::common;
using namespace gaia::db::messages;
using namespace flatbuffers;

class invalid_session_transition : public gaia_exception
{
public:
    invalid_session_transition(const std::string& message)
        : gaia_exception(message)
    {
    }
};

class server
{
    friend gaia::db::locators* gaia::db::get_shared_locators();
    friend gaia::db::data* gaia::db::get_shared_data();

public:
    static void run(bool disable_persistence = false);
    static constexpr char c_disable_persistence_flag[] = "--disable-persistence";

private:
    // from https://www.man7.org/linux/man-pages/man2/eventfd.2.html
    static constexpr uint64_t c_max_semaphore_count = std::numeric_limits<uint64_t>::max() - 1;
    // This is arbitrary but seems like a reasonable starting point (pending benchmarks).
    static constexpr size_t c_stream_batch_size = 1 << 10;
    static inline int s_server_shutdown_eventfd = -1;
    static inline int s_listening_socket = -1;
    static inline std::shared_mutex s_locators_lock{};
    static inline int s_fd_data = -1;
    static inline data* s_data = nullptr;
    static inline int s_fd_locators = -1;
    static inline locators* s_shared_locators = nullptr;
    thread_local static inline log* s_log = nullptr;
    thread_local static inline gaia_txn_id_t s_txn_id = c_invalid_gaia_txn_id;
    static inline std::unique_ptr<persistent_store_manager> rdb{};
    thread_local static inline int s_session_socket = -1;
    thread_local static inline session_state_t s_session_state = session_state_t::DISCONNECTED;
    thread_local static inline bool s_session_shutdown = false;
    thread_local static inline int s_session_shutdown_eventfd = -1;
    thread_local static inline std::vector<std::thread> s_session_owned_threads{};
    static inline bool s_disable_persistence = false;

    // function pointer type that executes side effects of a state transition
    // REVIEW: replace void* with std::any?
    typedef void (*transition_handler_fn)(int* fds, size_t fd_count, session_event_t event, const void* event_data, session_state_t old_state, session_state_t new_state);
    static void handle_connect(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_begin_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_rollback_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_commit_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_decide_txn(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_client_shutdown(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_server_shutdown(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);
    static void handle_request_stream(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);

    struct transition_t
    {
        session_state_t new_state;
        transition_handler_fn handler;
    };

    struct valid_transition_t
    {
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

    static inline constexpr valid_transition_t s_valid_transitions[] = {
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

    static void recover_db();

    static sigset_t mask_signals();

    static void signal_handler(sigset_t sigset, int& signum);

    static void init_listening_socket();

    static bool authenticate_client_socket(int socket);

    static void client_dispatch_handler();

    static void session_handler(int session_socket);

    template <typename element_type>
    static void stream_producer_handler(int stream_socket, int cancel_eventfd, std::function<std::optional<element_type>()> generator_fn);

    template <typename element_type>
    static void start_stream_producer(int stream_socket, std::function<std::optional<element_type>()> generator_fn);

    static std::function<std::optional<gaia_id_t>()>
    get_id_generator_for_type(gaia_type_t type);

    static bool txn_commit();
};

} // namespace db
} // namespace gaia
