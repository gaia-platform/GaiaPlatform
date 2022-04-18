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
#include "txn_metadata.hpp"
#include "type_index.hpp"

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

    friend gaia::db::locators_t* gaia::db::get_locators();
    friend gaia::db::locators_t* gaia::db::get_locators_for_allocator();
    friend gaia::db::counters_t* gaia::db::get_counters();
    friend gaia::db::data_t* gaia::db::get_data();
    friend gaia::db::logs_t* gaia::db::get_logs();
    friend gaia::db::id_index_t* gaia::db::get_id_index();
    friend gaia::db::type_index_t* gaia::db::get_type_index();
    friend gaia::db::index::indexes_t* gaia::db::get_indexes();
    friend gaia::db::gaia_txn_id_t gaia::db::get_current_txn_id();
    friend gaia::db::txn_log_t* gaia::db::get_txn_log();
    friend gaia::db::memory_manager_t* gaia::db::get_memory_manager();
    friend gaia::db::chunk_manager_t* gaia::db::get_chunk_manager();

public:
    static void run(server_config_t server_conf);

private:
    static inline server_config_t s_server_conf{};

    // This is arbitrary but seems like a reasonable starting point (pending benchmarks).
    static constexpr size_t c_stream_batch_size{1UL << 10};

    // This is necessary to avoid VM exhaustion in the worst case where all
    // sessions are opened from a single process (we remap the 256GB data
    // segment for each session, so the 128TB of VM available to each process
    // would be exhausted by 512 sessions opened in a single process, but we
    // also create other large per-session mappings, so we need a large margin
    // of error, hence the choice of 128 for the session limit).
    // REVIEW: How much could we relax this limit if we revert to per-process
    // mappings of the data segment?
    static constexpr size_t c_session_limit{1UL << 7};

    static inline int s_server_shutdown_eventfd = -1;
    static inline int s_listening_socket = -1;

    // These thread objects are owned by the client dispatch thread.
    static inline std::vector<std::thread> s_session_threads{};

    static inline mapped_data_t<locators_t> s_shared_locators{};
    static inline mapped_data_t<counters_t> s_shared_counters{};
    static inline mapped_data_t<data_t> s_shared_data{};
    static inline mapped_data_t<logs_t> s_shared_logs{};
    static inline mapped_data_t<id_index_t> s_shared_id_index{};
    static inline mapped_data_t<type_index_t> s_shared_type_index{};
    static inline index::indexes_t s_global_indexes{};
    static inline std::unique_ptr<persistent_store_manager> s_persistent_store{};

    // These fields have transaction lifetime.
    thread_local static inline gaia_txn_id_t s_txn_id = c_invalid_gaia_txn_id;
    thread_local static inline log_offset_t s_txn_log_offset = c_invalid_log_offset;
    thread_local static inline std::vector<std::pair<gaia_txn_id_t, log_offset_t>> s_txn_logs_for_snapshot{};

    // Local snapshot for server-side transactions.
    thread_local static inline mapped_data_t<locators_t> s_local_snapshot_locators{};
    // Watermark that tracks how many log records have been used for the current snapshot instance.
    // This is used to permit the incremental updating of the snapshot.
    thread_local static inline size_t s_last_snapshot_processed_log_record_count{0};

    // The allocated status of each log offset is tracked in this bitmap. When
    // opening a new txn, each session thread must allocate an offset for its txn
    // log by setting a cleared bit in this bitmap. When txn log GC completes,
    // the log's allocated bit should be cleared.
    static inline std::array<
        std::atomic<uint64_t>, (c_max_logs + 1) / common::c_uint64_bit_count>
        s_allocated_log_offsets_bitmap{};

    // We use this offset to track the lowest-numbered log offset that has never
    // been allocated.
    // NB: We use size_t here rather than log_offset_t to avoid integer
    // overflow. A 64-bit atomically incremented counter cannot overflow in any
    // reasonable time.
    static inline std::atomic<size_t> s_next_unused_log_offset{1};

    // This is used by GC tasks on a session thread to cache chunk IDs for empty chunk deallocation.
    thread_local static inline std::unordered_map<
        chunk_offset_t, chunk_version_t>
        s_map_gc_chunks_to_versions{};

    // These fields have session lifetime.
    thread_local static inline int s_session_socket = -1;
    thread_local static inline messages::session_state_t s_session_state = messages::session_state_t::DISCONNECTED;
    thread_local static inline bool s_session_shutdown = false;
    thread_local static inline int s_session_shutdown_eventfd = -1;

    thread_local static inline gaia::db::memory_manager_t s_memory_manager{};
    thread_local static inline gaia::db::chunk_manager_t s_chunk_manager{};

    thread_local static inline bool s_is_ddl_session{false};

    // These thread objects are owned by the session thread that created them.
    thread_local static inline std::vector<std::thread> s_session_owned_threads{};

    thread_local static inline std::string s_error_message = common::c_empty_string;

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

    // An array of monotonically nondecreasing timestamps, or "watermarks", that
    // represent the progress of system maintenance tasks with respect to txn
    // history. See `watermark_type_t` for a full explanation.
    static inline std::array<std::atomic<gaia_txn_id_t::value_type>, common::get_enum_value(watermark_type_t::count)> s_watermarks{};

    // A global array in which each session thread publishes a "safe timestamp"
    // that it needs to protect from memory reclamation. The minimum of all
    // published "safe timestamps" is an upper bound below which all txn
    // metadata entries can be safely reclaimed.
    //
    // Each thread maintains 2 publication entries rather than 1, in order to
    // tolerate validation failures. See the reserve_safe_ts() implementation
    // for a full explanation.
    //
    // Before using the safe_ts API, a thread must first reserve an index in
    // this array using reserve_safe_ts_index(). When a thread is no longer
    // using the safe_ts API (e.g., at session exit), it should call
    // release_safe_ts_index() to make this index available for use by other
    // threads.
    //
    // The only clients of the safe_ts API are expected to be session threads,
    // so we only allocate enough entries for the maximum allowed number of
    // session threads.
    static inline std::array<std::array<std::atomic<gaia_txn_id_t::value_type>, 2>, c_session_limit>
        s_safe_ts_per_thread_entries{};

    // The reserved status of each index into `s_safe_ts_per_thread_entries` is
    // tracked in this bitmap. Before calling any safe_ts API functions, each
    // thread must reserve an index by setting a cleared bit in this bitmap.
    // When it is no longer using the safe_ts API (e.g., at session exit), each
    // thread should clear the bit that it set.
    static inline std::array<
        std::atomic<uint64_t>, s_safe_ts_per_thread_entries.size() / common::c_uint64_bit_count>
        s_safe_ts_reserved_indexes_bitmap{};

    static constexpr size_t c_invalid_safe_ts_index{s_safe_ts_per_thread_entries.size()};

    // The current thread's index in `s_safe_ts_per_thread_entries`.
    thread_local static inline size_t s_safe_ts_index{c_invalid_safe_ts_index};

private:
    // Returns the current value of the given watermark.
    static inline gaia_txn_id_t get_watermark(watermark_type_t watermark_type)
    {
        return s_watermarks[common::get_enum_value(watermark_type)].load();
    }

    // Returns a reference to the array entry of the given watermark.
    static inline std::atomic<gaia_txn_id_t::value_type>& get_watermark_entry(watermark_type_t watermark_type)
    {
        return s_watermarks[common::get_enum_value(watermark_type)];
    }

    // Atomically advances the given watermark to the given timestamp, if the
    // given timestamp is larger than the watermark's current value. It thus
    // guarantees that the watermark is monotonically nondecreasing in time.
    //
    // Returns true if the watermark was advanced to the given value, false
    // otherwise.
    static bool advance_watermark(watermark_type_t watermark_type, gaia_txn_id_t ts);

    // Reserves an index for the current thread in
    // `s_safe_ts_per_thread_entries`. The entries at this index are read-write
    // for the current thread and read-only for all other threads.
    //
    // Returns true if an index was successfully reserved, false otherwise.
    static bool reserve_safe_ts_index();

    // Releases the current thread's index in `s_safe_ts_per_thread_entries`,
    // allowing the entries at that index to be used by other threads.
    //
    // This method cannot fail or throw.
    static void release_safe_ts_index();

    // Reserves a "safe timestamp" that protects its own and all subsequent
    // metadata entries from memory reclamation, by ensuring that the
    // pre-truncate watermark can never advance past it until release_safe_ts()
    // is called on that timestamp.
    //
    // Any previously reserved "safe timestamp" value for this thread will be
    // atomically replaced by the new "safe timestamp" value.
    //
    // This operation can fail if post-publication validation fails. See the
    // reserve_safe_ts() implementation for details.
    //
    // Returns true if the given timestamp was successfully reserved, false
    // otherwise.
    static bool reserve_safe_ts(gaia_txn_id_t safe_ts);

    // Releases the current thread's "safe timestamp", allowing the pre-truncate
    // watermark to advance past it, signaling to GC tasks that its metadata
    // entry is eligible for reclamation.
    //
    // This method cannot fail or throw.
    static void release_safe_ts();

    // This method computes a "safe truncation timestamp", i.e., an (exclusive)
    // upper bound below which all txn metadata entries can be safely reclaimed.
    //
    // The timestamp returned is guaranteed not to exceed any "safe timestamp"
    // that was reserved before this method was called.
    static gaia_txn_id_t get_safe_truncation_ts();

    // Returns true if the given log offset is allocated, false otherwise.
    static bool is_log_offset_allocated(log_offset_t offset);

    // Allocates the first unallocated log offset, returning the invalid log
    // offset if no unallocated offset is available.
    static log_offset_t allocate_log_offset();

    // Deallocates the given log offset.
    static void deallocate_log_offset(log_offset_t offset);

    // Allocates the first used log offset.
    static log_offset_t allocate_used_log_offset();

    // Allocates the first unused log offset.
    static log_offset_t allocate_unused_log_offset();

private:
    // A list of data mappings that we manage together.
    // The order of declarations must be the order of data_mapping_t::index_t values!
    static inline constexpr data_mapping_t c_data_mappings[] = {
        {data_mapping_t::index_t::locators, &s_shared_locators, c_gaia_mem_locators_prefix},
        {data_mapping_t::index_t::counters, &s_shared_counters, c_gaia_mem_counters_prefix},
        {data_mapping_t::index_t::data, &s_shared_data, c_gaia_mem_data_prefix},
        {data_mapping_t::index_t::logs, &s_shared_logs, c_gaia_mem_logs_prefix},
        {data_mapping_t::index_t::id_index, &s_shared_id_index, c_gaia_mem_id_index_prefix},
        {data_mapping_t::index_t::type_index, &s_shared_type_index, c_gaia_mem_type_index_prefix},
    };

    // Function pointer type that executes side effects of a session state transition.
    // REVIEW: replace void* with std::any?
    typedef void (*transition_handler_fn)(
        messages::session_event_t event,
        const void* event_data,
        messages::session_state_t old_state,
        messages::session_state_t new_state);

    // Session state transition handler functions.
    static void handle_connect_ddl(messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_connect(messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_begin_txn(messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_rollback_txn(messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_commit_txn(messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_decide_txn(messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_client_shutdown(messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_server_shutdown(messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);
    static void handle_request_stream(messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);

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
        {messages::session_state_t::DISCONNECTED, messages::session_event_t::CONNECT_DDL, {messages::session_state_t::CONNECTED, handle_connect_ddl}},
        {messages::session_state_t::DISCONNECTED, messages::session_event_t::CONNECT, {messages::session_state_t::CONNECTED, handle_connect}},
        {messages::session_state_t::ANY, messages::session_event_t::CLIENT_SHUTDOWN, {messages::session_state_t::DISCONNECTED, handle_client_shutdown}},
        {messages::session_state_t::CONNECTED, messages::session_event_t::BEGIN_TXN, {messages::session_state_t::TXN_IN_PROGRESS, handle_begin_txn}},
        {messages::session_state_t::TXN_IN_PROGRESS, messages::session_event_t::ROLLBACK_TXN, {messages::session_state_t::CONNECTED, handle_rollback_txn}},
        {messages::session_state_t::TXN_IN_PROGRESS, messages::session_event_t::COMMIT_TXN, {messages::session_state_t::TXN_COMMITTING, handle_commit_txn}},
        {messages::session_state_t::TXN_COMMITTING, messages::session_event_t::DECIDE_TXN_COMMIT, {messages::session_state_t::CONNECTED, handle_decide_txn}},
        {messages::session_state_t::TXN_COMMITTING, messages::session_event_t::DECIDE_TXN_ABORT, {messages::session_state_t::CONNECTED, handle_decide_txn}},
        {messages::session_state_t::TXN_COMMITTING, messages::session_event_t::DECIDE_TXN_ROLLBACK_FOR_ERROR, {messages::session_state_t::CONNECTED, handle_decide_txn}},
        {messages::session_state_t::ANY, messages::session_event_t::SERVER_SHUTDOWN, {messages::session_state_t::DISCONNECTED, handle_server_shutdown}},
        {messages::session_state_t::ANY, messages::session_event_t::REQUEST_STREAM, {messages::session_state_t::ANY, handle_request_stream}},
    };

    static void apply_transition(messages::session_event_t event, const void* event_data);

    static void build_server_reply_info(
        flatbuffers::FlatBufferBuilder& builder,
        messages::session_event_t event,
        messages::session_state_t old_state,
        messages::session_state_t new_state,
        gaia_txn_id_t txn_id = c_invalid_gaia_txn_id,
        log_offset_t txn_log_offset = c_invalid_log_offset,
        const std::vector<std::pair<gaia_txn_id_t, log_offset_t>>& txn_logs_to_apply = {});

    static void build_server_reply_error(
        flatbuffers::FlatBufferBuilder& builder,
        messages::session_event_t event,
        messages::session_state_t old_state,
        messages::session_state_t new_state,
        const char* error_message);

    static void clear_server_state();

    static void init_memory_manager(bool initializing);

    static void init_shared_memory();

    // Server-side index maintenance methods.

    // Initialize index maintenance on startup.
    static void init_indexes();

    // Update in-memory indexes based on the txn log.
    static void update_indexes_from_txn_log();

    // TODO: Remove the apply_logs flag, because a snapshot can't be used
    // correctly without first applying txn logs up to its begin timestamp.
    static void create_or_refresh_local_snapshot(bool apply_logs);

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
        int stream_socket,
        int cancel_eventfd,
        std::shared_ptr<common::iterators::generator_t<T_element>> generator_fn);

    template <typename T_element>
    static void start_stream_producer(
        int stream_socket,
        std::shared_ptr<common::iterators::generator_t<T_element>> generator);

    static void get_txn_log_offsets_for_snapshot(
        gaia_txn_id_t begin_ts,
        std::vector<std::pair<gaia_txn_id_t, log_offset_t>>& txn_ids_with_log_offsets_for_snapshot);

    static void release_txn_log_offsets_for_snapshot();

    static void release_transaction_resources();

    static void txn_begin();

    static void txn_rollback(bool client_disconnected = false);

    static bool txn_commit();

    static void perform_maintenance();

    static void apply_txn_logs_to_shared_view();

    static void gc_applied_txn_logs();

    static void update_post_gc_watermark();

    static void truncate_txn_table();

    static gaia_txn_id_t submit_txn(gaia_txn_id_t begin_ts, log_offset_t log_offset);

    static bool txn_logs_conflict(log_offset_t offset_1, log_offset_t offset_2);

    static void perform_pre_commit_work_for_txn();

    static bool validate_txn(gaia_txn_id_t commit_ts);

    static void validate_txns_in_range(gaia_txn_id_t start_ts, gaia_txn_id_t end_ts);

    static void apply_txn_log_from_ts(gaia_txn_id_t commit_ts);

    static void gc_txn_log_from_offset(log_offset_t offset, bool is_committed);

    static void deallocate_txn_log(txn_log_t* txn_log, bool deallocate_new_offsets);

    // The following method pairs are used to work on a startup transaction.

    // This method allocates a new begin_ts and initializes its entry in the txn
    // table. Returns the allocated txn_id.
    static gaia_txn_id_t begin_startup_txn();

    // This method creates a corresponding commit_ts to the txn above and initializes
    // their entries in the txn table.
    static void end_startup_txn();

    static void sort_log();

    static void deallocate_object(gaia_offset_t offset);

    static char* get_txn_metadata_page_address_from_ts(gaia_txn_id_t ts);

    static inline bool acquire_txn_log_reference_from_commit_ts(gaia_txn_id_t commit_ts);

    static inline void release_txn_log_reference_from_commit_ts(gaia_txn_id_t commit_ts);

    class safe_ts_failure : public common::gaia_exception
    {
    };

    // This class enables client code that is scanning a range of txn metadata
    // to prevent any of that metadata's memory from being reclaimed during the
    // scan. The client constructs a safe_ts_t object using the timestamp at the
    // left endpoint of the scan interval, which will prevent the pre-truncate
    // watermark from advancing past that timestamp until the safe_ts_t object
    // exits its scope, ensuring that all txn metadata at or after that "safe
    // timestamp" is protected from memory reclamation for the duration of the
    // scan.
    //
    // The constructor adds its timestamp argument to a thread-local set
    // containing the timestamps of all currently active safe_ts_t instances
    // owned by this thread. The minimum timestamp in the set is recomputed, and
    // if it has changed, the constructor attempts to reserve the new minimum
    // timestamp for this thread. If reservation fails, then the constructor
    // throws a `safe_ts_failure` exception.
    //
    // The destructor removes the object's timestamp from the thread-local set,
    // causing the minimum timestamp in the set to be recomputed. If it has
    // changed, the new minimum timestamp is reserved for this thread.
    // (Reservation cannot fail, because the previous minimum timestamp must be
    // smaller than the new minimum timestamp, so the pre-truncate watermark
    // could not have advanced beyond the new minimum timestamp.)
    class safe_ts_t
    {
    public:
        inline explicit safe_ts_t(gaia_txn_id_t safe_ts);
        inline ~safe_ts_t();
        safe_ts_t() = default;
        safe_ts_t(const safe_ts_t&) = delete;
        safe_ts_t& operator=(const safe_ts_t&) = delete;
        inline safe_ts_t(safe_ts_t&& other) noexcept;
        inline safe_ts_t& operator=(safe_ts_t&& other) noexcept;
        inline operator gaia_txn_id_t() const;

    private:
        // This member is logically immutable, but it cannot be `const` because
        // it needs to be set to an invalid value by the move
        // constructor/assignment operator.
        gaia_txn_id_t m_ts{c_invalid_gaia_txn_id};

        // We keep this vector sorted for fast searches. We add entries by
        // appending them and remove entries by swapping them with the tail.
        thread_local static inline std::vector<gaia_txn_id_t> s_safe_ts_values{};
    };

    // This class can be used in place of safe_ts_t, when the "safe timestamp"
    // value is just the current value of a watermark. Unlike the safe_ts_t
    // class, this class's constructor cannot fail, so clients do not need to
    // handle exceptions from initialization failure. It should be preferred to
    // safe_ts_t for protecting a range of txn metadata during a scan, whenever
    // the left endpoint of the scan interval is just the current value of a
    // watermark.
    class safe_watermark_t
    {
    public:
        inline explicit safe_watermark_t(watermark_type_t watermark);
        safe_watermark_t() = delete;
        ~safe_watermark_t() = default;
        safe_watermark_t(const safe_watermark_t&) = delete;
        safe_watermark_t& operator=(const safe_watermark_t&) = delete;
        safe_watermark_t(safe_watermark_t&&) = delete;
        safe_watermark_t& operator=(safe_watermark_t&&) = delete;
        inline operator gaia_txn_id_t() const;

    private:
        // This member is logically immutable, but it cannot be `const`, because
        // the constructor needs to move a local variable into it.
        safe_ts_t m_safe_ts;
    };
};

#include "db_server.inc"

} // namespace db
} // namespace gaia
