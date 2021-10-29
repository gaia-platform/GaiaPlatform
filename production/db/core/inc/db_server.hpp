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

#include <flatbuffers/flatbuffers.h>

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
        e_enabled,
        e_disabled,
        e_disabled_after_recovery,
        e_reinitialized_on_startup,
    };

    static constexpr persistence_mode_t c_default_persistence_mode = persistence_mode_t::e_enabled;

public:
    server_config_t(server_config_t::persistence_mode_t persistence_mode, std::string instance_name, std::string data_dir, bool testing)
        : m_persistence_mode(persistence_mode), m_instance_name(std::move(instance_name)), m_data_dir(std::move(data_dir)), m_skip_catalog_integrity_checks(testing)
    {
    }

    inline persistence_mode_t persistence_mode();
    inline const std::string& instance_name();
    inline const std::string& data_dir();
    inline bool skip_catalog_integrity_checks();

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
    bool m_skip_catalog_integrity_checks;
};

class server_t
{
    friend class gaia_ptr_t;
    friend class type_generator_t;

    friend gaia::db::locators_t* gaia::db::get_locators();
    friend gaia::db::locators_t* gaia::db::get_locators_for_allocator();
    friend gaia::db::counters_t* gaia::db::get_counters();
    friend gaia::db::data_t* gaia::db::get_data();
    friend gaia::db::id_index_t* gaia::db::get_id_index();
    friend gaia::db::gaia_txn_id_t gaia::db::get_current_txn_id();
    friend gaia::db::index::indexes_t* gaia::db::get_indexes();

public:
    // This needs to be public to be accessible from gaia::db::get_memory_manager().
    // This field has session lifetime.
    thread_local static inline gaia::db::memory_manager::memory_manager_t s_memory_manager{};
    // This needs to be public to be accessible from gaia::db::get_chunk_manager().
    // This field has session lifetime.
    thread_local static inline gaia::db::memory_manager::chunk_manager_t s_chunk_manager{};
    // This needs to be public to be accessible from gaia::db::get_mapped_log().
    // This field has transaction lifetime.
    thread_local static inline mapped_log_t s_log{};

public:
    static void run(server_config_t server_conf);

private:
    static inline server_config_t s_server_conf{};

    // This is arbitrary but seems like a reasonable starting point (pending benchmarks).
    static constexpr size_t c_stream_batch_size{1ULL << 10};

    // This is necessary to avoid VM exhaustion in the worst case where all
    // sessions are opened from a single process (we remap the 256GB data
    // segment for each session, so the 128TB of VM available to each process
    // would be exhausted by 512 sessions opened in a single process, but we
    // also create other large per-session mappings, so we need a large margin
    // of error, hence the choice of 128 for the session limit).
    static constexpr size_t c_session_limit{1ULL << 7};

    static inline int s_server_shutdown_eventfd = -1;
    static inline int s_listening_socket = -1;

    // These thread objects are owned by the client dispatch thread.
    static inline std::vector<std::thread> s_session_threads{};

    static inline mapped_data_t<locators_t> s_shared_locators{};
    static inline mapped_data_t<counters_t> s_shared_counters{};
    static inline mapped_data_t<data_t> s_shared_data{};
    static inline mapped_data_t<id_index_t> s_shared_id_index{};
    static inline index::indexes_t s_global_indexes{};
    static inline std::unique_ptr<persistent_store_manager> s_persistent_store{};

    // These fields have transaction lifetime.
    thread_local static inline gaia_txn_id_t s_txn_id = c_invalid_gaia_txn_id;

    // Local snapshot. This is a private copy of locators for server-side transactions.
    thread_local static inline mapped_data_t<locators_t> s_local_snapshot_locators{};

    // This is used by GC tasks on a session thread to cache chunk IDs for empty chunk deallocation.
    thread_local static inline std::unordered_map<
        memory_manager::chunk_offset_t, memory_manager::chunk_version_t>
        s_map_gc_chunks_to_versions{};

    // These fields have session lifetime.
    thread_local static inline int s_session_socket = -1;
    thread_local static inline messages::session_state_t s_session_state = messages::session_state_t::DISCONNECTED;
    thread_local static inline bool s_session_shutdown = false;
    thread_local static inline int s_session_shutdown_eventfd = -1;

    // These thread objects are owned by the session thread that created them.
    thread_local static inline std::vector<std::thread> s_session_owned_threads{};

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
    // could have had GC reclaim all its resources. Finally, the "pre-truncate"
    // watermark represents an (exclusive) upper bound on the timestamps whose
    // metadata entries could have had their memory reclaimed (e.g., via
    // zeroing, unmapping, or overwriting). Any timestamp whose metadata entry
    // could potentially be dereferenced must be "reserved" via the "safe_ts"
    // API to prevent the pre-truncate watermark from advancing past it and
    // allowing its metadata entry to be reclaimed.
    //
    // The pre-apply watermark must either be equal to the post-apply watermark or greater by 1.
    //
    // Schematically:
    //    commit timestamps of transactions whose metadata entries have been reclaimed
    //  < pre-truncate watermark
    //    <= commit timestamps of transactions completely garbage-collected
    // <= post-GC watermark
    //    <= commit timestamps of transactions applied to shared view
    // <= post-apply watermark
    //    < commit timestamp of transaction partially applied to shared view
    // <= pre-apply watermark
    //    < commit timestamps of transactions not applied to shared view.

    enum class watermark_type_t
    {
        pre_apply,
        post_apply,
        post_gc,
        pre_truncate,
        // This should always be last.
        count
    };

    static inline std::array<std::atomic<gaia_txn_id_t>, common::get_enum_value(watermark_type_t::count)> s_watermarks{};

    static inline gaia_txn_id_t get_watermark(watermark_type_t watermark)
    {
        return s_watermarks[common::get_enum_value(watermark)].load();
    }

    static inline std::atomic<gaia_txn_id_t>& get_watermark_entry(watermark_type_t watermark)
    {
        return s_watermarks[common::get_enum_value(watermark)];
    }

    static bool advance_watermark(watermark_type_t watermark, gaia_txn_id_t ts);

    // A "publication list" in which each session thread publishes a "safe
    // timestamp" that it needs to protect from txn table truncation. This is
    // used to find a safe upper bound at which to truncate the txn table.
    //
    // Each thread reserves 2 entries rather than 1, because it needs to
    // atomically update its published entry. This requires speculatively
    // publishing the new value while keeping the old value visible, so that the
    // old value continues to be protected if the new value fails validation.
    // Therefore we require 2 entries: one to hold the old value, and another to
    // hold the new value until it either passes validation (and we invalidate
    // the old value's entry), or fails validation (and we invalidate the new
    // value's entry, reverting the published safe_ts for this thread to the old
    // value).
    static inline std::array<std::array<std::atomic<gaia_txn_id_t>, 2>, c_session_limit>
        s_safe_ts_per_thread_entries{};

    // The reserved status of each index into `s_safe_ts_per_thread_entries` is
    // tracked in this bitmap. Before calling any safe_ts API functions, each
    // thread must reserve an index by setting a cleared bit in this bitmap.
    // When it is no longer using the safe_ts API (e.g., at session exit), each
    // thread should clear the bit that it set.
    static constexpr size_t c_safe_ts_reserved_indexes_bitmap_size_in_words{
        c_session_limit / memory_manager::c_uint64_bit_count};

    static inline std::array<std::atomic<uint64_t>, c_safe_ts_reserved_indexes_bitmap_size_in_words>
        s_safe_ts_reserved_indexes_bitmap{};

    static constexpr size_t c_invalid_safe_ts_index{std::size(s_safe_ts_per_thread_entries)};

    // The current thread's index in `s_safe_ts_per_thread_entries`.
    thread_local static inline size_t s_safe_ts_index{c_invalid_safe_ts_index};

    // Reserves an index for the current thread in `s_safe_ts_per_thread_entries`.
    static bool reserve_safe_ts_index();

    // Releases the current thread's index in `s_safe_ts_per_thread_entries`.
    static void release_safe_ts_index();

    // Reserves a "safe timestamp" that protects its own and all subsequent
    // entries in the txn table from memory reclamation.
    // Any current "safe timestamp" value for this thread's entry will be
    // atomically replaced by the new "safe timestamp" value.
    // If the given timestamp cannot be reserved as "safe", then return false,
    // otherwise true.
    static bool reserve_safe_ts(gaia_txn_id_t safe_ts);

    // Releases the current thread's "safe timestamp", allowing that timestamp's
    // metadata entry (and subsequent entries) to be reclaimed.
    // This method cannot fail or throw.
    static void release_safe_ts();

    // This method computes a "safe truncation timestamp" for the txn table,
    // i.e., an (exclusive) upper bound below which the txn table can be safely
    // truncated. The timestamp returned is guaranteed not to exceed any
    // safe_ts that was reserved before this method was called.
    static gaia_txn_id_t get_safe_truncation_ts();

private:
    // A list of data mappings that we manage together.
    // The order of declarations must be the order of data_mapping_t::index_t values!
    static inline constexpr data_mapping_t c_data_mappings[] = {
        {data_mapping_t::index_t::locators, &s_shared_locators, c_gaia_mem_locators_prefix},
        {data_mapping_t::index_t::counters, &s_shared_counters, c_gaia_mem_counters_prefix},
        {data_mapping_t::index_t::data, &s_shared_data, c_gaia_mem_data_prefix},
        {data_mapping_t::index_t::id_index, &s_shared_id_index, c_gaia_mem_id_index_prefix},
    };

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
        {messages::session_state_t::TXN_COMMITTING, messages::session_event_t::DECIDE_TXN_ROLLBACK_UNIQUE, {messages::session_state_t::CONNECTED, handle_decide_txn}},
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
        size_t log_fds_to_apply_count = 0);

    static void clear_shared_memory();

    static void init_memory_manager(bool initializing);

    static void init_shared_memory();

    // Server-side index maintenance methods.

    // Initialize index maintenance on startup.
    static void init_indexes();

    // Update in-memory indexes based on the txn log.
    static void update_indexes_from_txn_log();

    static void create_local_snapshot(bool apply_logs);

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

    static void txn_begin(std::vector<int>& txn_log_fds_for_snapshot);

    static void txn_rollback(bool client_disconnected = false);

    static bool txn_commit();

    static void perform_maintenance();

    static void apply_txn_logs_to_shared_view();

    static void gc_applied_txn_logs();

    static void truncate_txn_table();

    static gaia_txn_id_t submit_txn(gaia_txn_id_t begin_ts, int log_fd);

    static bool txn_logs_conflict(int log_fd1, int log_fd2);

    static void perform_pre_commit_work_for_txn();

    static bool validate_txn(gaia_txn_id_t commit_ts);

    static void validate_txns_in_range(gaia_txn_id_t start_ts, gaia_txn_id_t end_ts);

    static void apply_txn_log_from_ts(gaia_txn_id_t commit_ts);

    static void gc_txn_log_from_fd(int log_fd, bool committed = true);

    static void deallocate_txn_log(txn_log_t* txn_log, bool deallocate_new_offsets = false);

    // The following method pairs are used to work on a startup transaction.

    // This method allocates a new begin_ts and initializes its entry in the txn
    // table. Returns the allocated txn_id.
    static gaia_txn_id_t begin_startup_txn();

    // This method creates a corresponding commit_ts to the txn above and initializes
    // their entries in the txn table.
    static void end_startup_txn();

    static void sort_log();

    static void deallocate_object(gaia_offset_t offset);

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

    // This class allows txn code to safely use a txn log fd embedded in a
    // commit_ts metadata entry even while it is concurrently invalidated (i.e.,
    // the fd is closed and its embedded value is set to -1). The constructor
    // throws an invalid_log_fd exception if the fd is invalidated during
    // construction.
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
