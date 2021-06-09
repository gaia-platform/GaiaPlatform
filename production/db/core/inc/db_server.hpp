/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <csignal>

#include <atomic>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <utility>

#include "flatbuffers/flatbuffers.h"

#include "gaia/exception.hpp"

#include "gaia_internal/common/generator_iterator.hpp"

#include "db_internal_types.hpp"
#include "mapped_data.hpp"
#include "memory_manager.hpp"
#include "messages_generated.h"
#include "persistent_store_manager.hpp"

namespace gaia
{
namespace db
{

class invalid_session_transition : public common::gaia_exception
{
public:
    explicit invalid_session_transition(const std::string& message)
        : gaia_exception(message)
    {
    }
};

/**
 * Encapsulates the server_t configuration.
 */
class server_config_t
{
    friend class server_t;

public:
    enum class persistence_mode_t : uint8_t
    {
        e_default,
        e_disabled,
        e_disabled_after_recovery,
        e_reinitialized_on_startup,
    };

    static constexpr persistence_mode_t c_default_persistence_mode = persistence_mode_t::e_default;

public:
    server_config_t(server_config_t::persistence_mode_t persistence_mode, std::string instance_name, std::string data_dir)
        : m_persistence_mode(persistence_mode), m_instance_name(std::move(instance_name)), m_data_dir(std::move(data_dir))
    {
    }

    inline persistence_mode_t persistence_mode();
    inline const std::string& instance_name();
    inline const std::string& data_dir();

private:
    // Dummy constructor to allow server_t initialization.
    server_config_t()
        : m_persistence_mode(c_default_persistence_mode)
    {
    }

private:
    persistence_mode_t m_persistence_mode;
    std::string m_instance_name;
    std::string m_data_dir;
};

class server_t
{
    friend gaia::db::locators_t* gaia::db::get_locators();
    friend gaia::db::counters_t* gaia::db::get_counters();
    friend gaia::db::data_t* gaia::db::get_data();
    friend gaia::db::id_index_t* gaia::db::get_id_index();

    friend gaia::db::memory_manager::address_offset_t gaia::db::allocate_object(
        gaia_locator_t locator,
        size_t size);

public:
    static void run(server_config_t server_conf);
    static void register_object_deallocator(std::function<void(gaia_offset_t)>);

private:
    static inline server_config_t s_server_conf{};

    // This is arbitrary but seems like a reasonable starting point (pending benchmarks).
    static constexpr size_t c_stream_batch_size{1ULL << 10};

    static inline int s_server_shutdown_eventfd = -1;

    // These thread objects are owned by the client dispatch thread.
    // These fields have session lifetime.
    static inline std::vector<std::thread> s_session_threads{};
    static inline int s_listening_socket = -1;

    static inline mapped_data_t<locators_t> s_shared_locators{};
    static inline mapped_data_t<counters_t> s_shared_counters{};
    static inline mapped_data_t<data_t> s_shared_data{};
    static inline mapped_data_t<id_index_t> s_shared_id_index{};

    // These fields have transaction lifetime.
    thread_local static inline int s_fd_log = -1;
    thread_local static inline txn_log_t* s_log = nullptr;

    thread_local static inline gaia_txn_id_t s_txn_id = c_invalid_gaia_txn_id;

    static inline std::unique_ptr<persistent_store_manager> rdb{};

    thread_local static inline int s_session_socket = -1;
    thread_local static inline messages::session_state_t s_session_state = messages::session_state_t::DISCONNECTED;
    thread_local static inline bool s_session_shutdown = false;
    thread_local static inline int s_session_shutdown_eventfd = -1;

    // These thread objects are owned by the session thread that created them.
    thread_local static inline std::vector<std::thread> s_session_owned_threads{};

    static inline gaia::db::memory_manager::memory_manager_t s_memory_manager{};
    static inline gaia::db::memory_manager::chunk_manager_t s_chunk_manager{};

    // These global timestamp variables are "watermarks" that represent the
    // progress of various system functions with respect to transaction history.
    // The "pre-apply" watermark represents an upper bound on the latest
    // commit_ts whose txn log could have been applied to the shared locator
    // view. A committed txn cannot have its txn log applied to the shared view
    // until the pre-apply watermark has been advanced to its commit_ts. The
    // "post-apply" watermark represents a lower bound on the same quantity, and
    // also an upper bound on the latest commit_ts whose txn log could be
    // eligible for GC. GC cannot be started for any committed txn until the
    // post-apply watermark has advanced to its commit_ts. The "post-GC"
    // watermark represents a lower bound on the latest commit_ts whose txn log
    // could have had GC reclaim all its resources. The txn table cannot be
    // truncated at any timestamp after the post-GC watermark.
    //
    // The pre-apply watermark must either be equal to the post-apply watermark or greater by 1.
    //
    // Schematically:
    //    commit timestamps of transactions completely garbage-collected
    // <= post-GC watermark
    //    <= commit timestamps of transactions applied to shared view
    // <= post-apply watermark
    //    < commit timestamp of transaction partially applied to shared view
    // <= pre-apply watermark
    //    < commit timestamps of transactions not applied to shared view.
    static inline std::atomic<gaia_txn_id_t> s_last_applied_commit_ts_upper_bound = c_invalid_gaia_txn_id;
    static inline std::atomic<gaia_txn_id_t> s_last_applied_commit_ts_lower_bound = c_invalid_gaia_txn_id;
    static inline std::atomic<gaia_txn_id_t> s_last_freed_commit_ts_lower_bound = c_invalid_gaia_txn_id;

    // This is an extension point called by the transactional system when the
    // "watermark" advances (i.e., the oldest active txn terminates or commits),
    // allowing all the superseded object versions in txns with commit
    // timestamps before the watermark to be freed (because they are no longer
    // visible to any present or future txns).
    static inline std::function<void(gaia_offset_t)> s_object_deallocator_fn{};

private:
    // Function pointer type that executes side effects of a session state transition.
    // REVIEW: replace void* with std::any?
    typedef void (*transition_handler_fn)(
        int* fds, size_t fd_count,
        messages::session_event_t event,
        const void* event_data,
        messages::session_state_t old_state,
        messages::session_state_t new_state);

    // Session state transition handler functions.
    static void handle_connect(int*, size_t, messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_begin_txn(int*, size_t, messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_rollback_txn(int*, size_t, messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_commit_txn(int*, size_t, messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_decide_txn(int*, size_t, messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_client_shutdown(int*, size_t, messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_server_shutdown(int*, size_t, messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_request_stream(int*, size_t, messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);

    struct transition_t
    {
        messages::session_state_t new_state;
        transition_handler_fn handler;
    };

    struct valid_transition_t
    {
        messages::session_state_t state;
        messages::session_event_t event;
        transition_t transition;
    };

    // "Wildcard" transitions (current state = session_state_t::ANY) must be listed after
    // non-wildcard transitions with the same event, or the latter will never be applied.
    static inline constexpr valid_transition_t c_valid_transitions[] = {
        {messages::session_state_t::DISCONNECTED, messages::session_event_t::CONNECT, {messages::session_state_t::CONNECTED, handle_connect}},
        {messages::session_state_t::ANY, messages::session_event_t::CLIENT_SHUTDOWN, {messages::session_state_t::DISCONNECTED, handle_client_shutdown}},
        {messages::session_state_t::CONNECTED, messages::session_event_t::BEGIN_TXN, {messages::session_state_t::TXN_IN_PROGRESS, handle_begin_txn}},
        {messages::session_state_t::TXN_IN_PROGRESS, messages::session_event_t::ROLLBACK_TXN, {messages::session_state_t::CONNECTED, handle_rollback_txn}},
        {messages::session_state_t::TXN_IN_PROGRESS, messages::session_event_t::COMMIT_TXN, {messages::session_state_t::TXN_COMMITTING, handle_commit_txn}},
        {messages::session_state_t::TXN_COMMITTING, messages::session_event_t::DECIDE_TXN_COMMIT, {messages::session_state_t::CONNECTED, handle_decide_txn}},
        {messages::session_state_t::TXN_COMMITTING, messages::session_event_t::DECIDE_TXN_ABORT, {messages::session_state_t::CONNECTED, handle_decide_txn}},
        {messages::session_state_t::ANY, messages::session_event_t::SERVER_SHUTDOWN, {messages::session_state_t::DISCONNECTED, handle_server_shutdown}},
        {messages::session_state_t::ANY, messages::session_event_t::REQUEST_STREAM, {messages::session_state_t::ANY, handle_request_stream}},
    };

    static void apply_transition(messages::session_event_t event, const void* event_data, int* fds, size_t fd_count);

    static void build_server_reply(
        flatbuffers::FlatBufferBuilder& builder,
        messages::session_event_t event,
        messages::session_state_t old_state,
        messages::session_state_t new_state,
        gaia_txn_id_t txn_id = 0,
        size_t log_fd_count = 0);

    static void clear_shared_memory();

    static void init_memory_manager();

    static void init_shared_memory();

    static void recover_db();

    static sigset_t mask_signals();

    static void signal_handler(sigset_t sigset, int& signum);

    static void init_listening_socket(const std::string& socket_name);

    static bool authenticate_client_socket(int socket);

    static void client_dispatch_handler(const std::string& socket_name);

    static void session_handler(int session_socket);

    static std::pair<int, int> get_stream_socket_pair();

    template <typename T_element>
    static void stream_producer_handler(
        int stream_socket, int cancel_eventfd, std::shared_ptr<common::iterators::generator_t<T_element>> generator_fn);

    template <typename T_element>
    static void start_stream_producer(int stream_socket, std::shared_ptr<common::iterators::generator_t<T_element>> generator);

    static std::shared_ptr<common::iterators::generator_t<common::gaia_id_t>> get_id_generator_for_type(common::gaia_type_t type);

    static void get_txn_log_fds_for_snapshot(gaia_txn_id_t begin_ts, std::vector<int>& txn_log_fds);

    static void txn_rollback();

    static bool txn_commit();

    static gaia::db::memory_manager::address_offset_t allocate_object(
        gaia_locator_t locator,
        size_t size);

    static void perform_maintenance();

    static void apply_txn_logs_to_shared_view();

    static void gc_applied_txn_logs();

    static void update_txn_table_safe_truncation_point();

    static bool advance_watermark(std::atomic<gaia_txn_id_t>& watermark, gaia_txn_id_t ts);

    static gaia_txn_id_t submit_txn(gaia_txn_id_t begin_ts, int log_fd);

    static bool txn_logs_conflict(int log_fd1, int log_fd2);

    static void perform_pre_commit_work_for_txn();

    static bool validate_txn(gaia_txn_id_t commit_ts);

    static void validate_txns_in_range(gaia_txn_id_t start_ts, gaia_txn_id_t end_ts);

    static void apply_txn_log_from_ts(gaia_txn_id_t commit_ts);

    static void gc_txn_log_from_fd(int log_fd, bool committed = true);

    static void deallocate_txn_log(txn_log_t* txn_log, bool deallocate_new_offsets = false);

    class invalid_log_fd : public common::gaia_exception
    {
    public:
        explicit invalid_log_fd(gaia_txn_id_t commit_ts)
            : m_commit_ts(commit_ts)
        {
        }

        gaia_txn_id_t get_ts() const
        {
            return m_commit_ts;
        }

    private:
        gaia_txn_id_t m_commit_ts{c_invalid_gaia_txn_id};
    };

    // This class allows txn code to safely use a txn log fd embedded in a commit_ts
    // metadata even while it is concurrently invalidated (i.e. the fd is closed and
    // its embedded value set to -1). The constructor throws an invalid_log_fd
    // exception if the fd is invalidated during construction.
    class safe_fd_from_ts_t
    {
    public:
        explicit safe_fd_from_ts_t(gaia_txn_id_t commit_ts, bool auto_close_fd = true);

        // The "rule of 3" doesn't apply here because we never pass this object
        // directly to a function; we always extract the safe fd value first.
        safe_fd_from_ts_t() = delete;
        safe_fd_from_ts_t(const safe_fd_from_ts_t&) = delete;
        safe_fd_from_ts_t& operator=(const safe_fd_from_ts_t&) = delete;

        ~safe_fd_from_ts_t();

        // This is the only public API. Callers are expected to call this method on
        // a stack-constructed instance of the class before they pass an fd to a
        // function that does not expect it to be concurrently closed.
        int get_fd() const;

    private:
        int m_local_log_fd{-1};
        bool m_auto_close_fd{true};
    };
};

#include "db_server.inc"

} // namespace db
} // namespace gaia
