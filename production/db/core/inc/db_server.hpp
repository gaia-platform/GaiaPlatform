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

#include "flatbuffers/flatbuffers.h"

#include "gaia/exception.hpp"

#include "gaia_internal/common/mmap_helpers.hpp"

#include "db_internal_types.hpp"
#include "memory_manager.hpp"
#include "messages_generated.h"
#include "persistent_store_manager.hpp"
#include "stack_allocator.hpp"

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

class server
{
    friend gaia::db::locators_t* gaia::db::get_locators();
    friend gaia::db::counters_t* gaia::db::get_counters();
    friend gaia::db::data_t* gaia::db::get_data();
    friend gaia::db::id_index_t* gaia::db::get_id_index();

    friend gaia::db::memory_manager::address_offset_t gaia::db::allocate_object(
        gaia_locator_t locator,
        size_t size);

public:
    enum class persistence_mode_t : uint8_t
    {
        e_default,
        e_disabled,
        e_disabled_after_recovery,
        e_reinitialized_on_startup,
    };

    static void run(persistence_mode_t persistence_mode = persistence_mode_t::e_default);

    static void register_object_deallocator(std::function<void(gaia_offset_t)>);

    static constexpr char c_disable_persistence_flag[] = "--disable-persistence";
    static constexpr char c_disable_persistence_after_recovery_flag[] = "--disable-persistence-after-recovery";
    static constexpr char c_reinitialize_persistent_store_flag[] = "--reinitialize-persistent-store";

private:
    // from https://www.man7.org/linux/man-pages/man2/eventfd.2.html
    static constexpr uint64_t c_max_semaphore_count = std::numeric_limits<uint64_t>::max() - 1;

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

    static inline persistence_mode_t s_persistence_mode{persistence_mode_t::e_default};

    static inline std::unique_ptr<gaia::db::memory_manager::memory_manager_t> s_memory_manager{};

    // This is an effectively infinite array of timestamp entries, indexed by
    // the txn timestamp counter and containing metadata for every txn that has
    // been submitted to the system.
    //
    // Entries may be "unknown" (uninitialized), "invalid" (initialized with a
    // special "junk" value and forbidden to be used afterward), or initialized
    // with txn metadata, consisting of 3 status bits, 1 bit for GC status
    // (unknown or complete), 1 bit for persistence status (unknown or
    // complete), 1 bit reserved for future use, 16 bits for a txn log fd, and
    // 42 bits for a linked timestamp (i.e., the commit timestamp of a submitted
    // txn embedded in its begin timestamp entry, or the begin timestamp of a
    // submitted txn embedded in its commit timestamp entry). The 3 status bits
    // use the high bit to distinguish begin timestamps from commit timestamps,
    // and 2 bits to store the state of an active, terminated, or submitted txn.
    //
    // The array is always accessed without any locking, but its entries have
    // read and write barriers (via std::atomic) that ensure causal consistency
    // between any threads that read or write the same entry. Any writes to
    // entries that may be written by multiple threads use CAS operations.
    //
    // The array's memory is managed via mmap(MAP_NORESERVE). We reserve 32TB of
    // virtual address space (1/8 of the total virtual address space available
    // to the process), but allocate physical pages only on first access. When a
    // range of timestamp entries falls behind the watermark, its physical pages
    // can be deallocated via madvise(MADV_FREE).

    // REVIEW: Since we reserve 2^45 bytes of virtual address space and each
    // array entry is 8 bytes, we can address the whole range using 2^42
    // timestamps. If we allocate 2^10 timestamps/second, we will use up all our
    // timestamps in 2^32 seconds, or about 2^7 years. If we allocate 2^20
    // timestamps/second, we will use up all our timestamps in 2^22 seconds, or
    // about a month and a half. If this is an issue, then we could treat the
    // array as a circular buffer, using a separate wraparound counter to
    // calculate the array offset from a timestamp, and we can use the 3
    // reserved bits in the timestamp entry to extend our range by a factor of
    // 8, so we could allocate 2^20 timestamps/second for a full year. If we
    // need a still larger timestamp range (say 64-bit timestamps, with
    // wraparound), we could just store the difference between a commit
    // timestamp and its txn's begin timestamp, which should be possible to
    // bound to no more than half the bits we use for the full timestamp, so we
    // would still need only 32 bits for a timestamp reference in the timestamp
    // entry. (We could store the array offset instead, but that would be
    // dangerous when we approach wraparound.)
    //
    // Timestamp entry format:
    // 64 bits: txn_status (3) | gc_status (1) | persistence_status (1) | reserved (1) | log_fd (16) | linked_timestamp (42)
    typedef uint64_t ts_entry_t;
    static inline std::atomic<ts_entry_t>* s_txn_info = nullptr;

    // This should always be true on any 64-bit platform, but we assert since we
    // never want to fall back to a lock-based implementation of atomics.
    static_assert(std::atomic<ts_entry_t>::is_always_lock_free);

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
    // truncated at any timestamp entry after the post-GC watermark.

    // Schematically:
    // commit timestamps of transactions completely garbage-collected
    // <= post-GC watermark
    // <= commit timestamps of transactions applied to shared view
    // <= post-apply watermark
    // < commit timestamps of transactions partially applied to shared view
    // <= pre-apply watermark
    // < commit timestamps of transactions not applied to shared view.

    static inline std::atomic<gaia_txn_id_t> s_last_applied_commit_ts_upper_bound = c_invalid_gaia_txn_id;
    static inline std::atomic<gaia_txn_id_t> s_last_applied_commit_ts_lower_bound = c_invalid_gaia_txn_id;
    static inline std::atomic<gaia_txn_id_t> s_last_freed_commit_ts_lower_bound = c_invalid_gaia_txn_id;

    // This is an extension point called by the transactional system when the
    // "watermark" advances (i.e., the oldest active txn terminates or commits),
    // allowing all the superseded object versions in txns with commit
    // timestamps before the watermark to be freed (since they are no longer
    // visible to any present or future txns).
    static inline std::function<void(gaia_offset_t)> s_object_deallocator_fn{};

    // Transaction timestamp entry constants.
    static constexpr uint64_t c_txn_entry_bits{64ULL};

    static constexpr uint64_t c_txn_status_flags_bits{3ULL};
    static constexpr uint64_t c_txn_status_flags_shift{c_txn_entry_bits - c_txn_status_flags_bits};
    static constexpr uint64_t c_txn_status_flags_mask{
        ((1ULL << c_txn_status_flags_bits) - 1) << c_txn_status_flags_shift};

    // Transaction status flags.
    // These are all begin_ts status values.
    static constexpr uint64_t c_txn_status_active{0b010ULL};
    static constexpr uint64_t c_txn_status_submitted{0b011ULL};
    static constexpr uint64_t c_txn_status_terminated{0b001ULL};

    // This is the bitwise intersection of all commit_ts status values.
    static constexpr uint64_t c_txn_status_commit_ts{0b100ULL};
    static constexpr uint64_t c_txn_status_commit_mask{
        c_txn_status_commit_ts << c_txn_status_flags_shift};

    // This is the bitwise intersection of all commit_ts decided status values
    // (i.e., committed or aborted).
    static constexpr uint64_t c_txn_status_decided{0b110ULL};
    static constexpr uint64_t c_txn_status_decided_mask{
        c_txn_status_decided << c_txn_status_flags_shift};

    // These are all commit_ts status values.
    static constexpr uint64_t c_txn_status_validating{0b100ULL};
    static constexpr uint64_t c_txn_status_committed{0b111ULL};
    static constexpr uint64_t c_txn_status_aborted{0b110ULL};

    // Transaction GC status values.
    // These only apply to a commit_ts entry.
    // We don't need TXN_GC_ELIGIBLE or TXN_GC_INITIATED flags, since any txn
    // behind the post-apply watermark (and with TXN_PERSISTENCE_COMPLETE set if
    // persistence is enabled) is eligible for GC, and an invalidated log fd
    // indicates that GC is in progress.
    static constexpr uint64_t c_txn_gc_flags_bits{1ULL};
    static constexpr uint64_t c_txn_gc_flags_shift{
        (c_txn_entry_bits - c_txn_gc_flags_bits) - c_txn_status_flags_bits};
    static constexpr uint64_t c_txn_gc_flags_mask{
        ((1ULL << c_txn_gc_flags_bits) - 1) << c_txn_gc_flags_shift};
    // These are all commit_ts flag values.
    static constexpr uint64_t c_txn_gc_unknown{0b0ULL};
    // This flag indicates that the txn log and all obsolete versions (undo
    // versions for a committed txn, redo versions for an aborted txn) have been
    // reclaimed by the system.
    static constexpr uint64_t c_txn_gc_complete{0b1ULL};

    // This flag indicates whether the txn has been made externally durable
    // (i.e., persisted to the write-ahead log). It can't be combined with the
    // GC flags because a txn might be made durable before or after being
    // applied to the shared view, and we don't want one to block on the other.
    // However, a committed txn's redo versions cannot be reclaimed until it has
    // been marked durable. If persistence is disabled, this flag is unused.
    static constexpr uint64_t c_txn_persistence_flags_bits{1ULL};
    static constexpr uint64_t c_txn_persistence_flags_shift{
        (c_txn_entry_bits - c_txn_persistence_flags_bits)
        - (c_txn_status_flags_bits + c_txn_gc_flags_bits)};
    static constexpr uint64_t c_txn_persistence_flags_mask{
        ((1ULL << c_txn_persistence_flags_bits) - 1) << c_txn_persistence_flags_shift};
    // These are all commit_ts flag values.
    static constexpr uint64_t c_txn_persistence_unknown{0b0ULL};
    static constexpr uint64_t c_txn_persistence_complete{0b1ULL};

    // This is a placeholder for the single (currently) reserved bit in the txn
    // timestamp entry.
    static constexpr uint64_t c_txn_reserved_flags_bits{1ULL};

    // Txn log fd embedded in the txn timestamp entry.
    // This is only present in a commit_ts entry.
    // NB: we assume that any fd will be < 2^16 - 1!
    static constexpr uint64_t c_txn_log_fd_bits{16ULL};
    static constexpr uint64_t c_txn_log_fd_shift{
        (c_txn_entry_bits - c_txn_log_fd_bits)
        - (c_txn_status_flags_bits
           + c_txn_gc_flags_bits
           + c_txn_persistence_flags_bits
           + c_txn_reserved_flags_bits)};
    static constexpr uint64_t c_txn_log_fd_mask{
        ((1ULL << c_txn_log_fd_bits) - 1) << c_txn_log_fd_shift};

    // Linked txn timestamp embedded in the txn timestamp entry.
    // For a commit_ts entry, this is its associated begin_ts, and for a
    // begin_ts entry, this is its associated commit_ts. The linked begin_ts is
    // always present in a commit_ts entry, but the associated begin_ts entry
    // may not be updated with its linked commit_ts until after the commit_ts
    // entry has been created.
    static constexpr uint64_t c_txn_ts_bits{42ULL};
    static constexpr uint64_t c_txn_ts_shift{0ULL};
    static constexpr uint64_t c_txn_ts_mask{((1ULL << c_txn_ts_bits) - 1) << c_txn_ts_shift};

    // Transaction special values.
    // The first 3 bits of this value are unused for any txn state.
    static constexpr ts_entry_t c_txn_entry_unknown{0ULL};

    // This must always be 0 because a newly-allocated page is initialized to 0.
    static_assert(c_txn_entry_unknown == 0);

    // The first 3 bits of this value are unused for any txn state.
    static constexpr ts_entry_t c_txn_entry_invalid{0b101ULL << c_txn_status_flags_shift};

    // Since we restrict all fds to 16 bits, this is the largest possible value
    // in that range, which we reserve to indicate an invalidated fd (i.e., one
    // which was claimed for deallocation by a maintenance thread).
    static constexpr uint16_t c_invalid_txn_log_fd_bits{std::numeric_limits<uint16_t>::max()};

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
    static void handle_request_memory(int*, size_t, messages::session_event_t, const void*, messages::session_state_t, messages::session_state_t);

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

    static inline constexpr valid_transition_t s_valid_transitions[] = {
        {messages::session_state_t::DISCONNECTED, messages::session_event_t::CONNECT, {messages::session_state_t::CONNECTED, handle_connect}},
        {messages::session_state_t::ANY, messages::session_event_t::CLIENT_SHUTDOWN, {messages::session_state_t::DISCONNECTED, handle_client_shutdown}},
        {messages::session_state_t::CONNECTED, messages::session_event_t::BEGIN_TXN, {messages::session_state_t::TXN_IN_PROGRESS, handle_begin_txn}},
        {messages::session_state_t::TXN_IN_PROGRESS, messages::session_event_t::ROLLBACK_TXN, {messages::session_state_t::CONNECTED, handle_rollback_txn}},
        {messages::session_state_t::TXN_IN_PROGRESS, messages::session_event_t::COMMIT_TXN, {messages::session_state_t::TXN_COMMITTING, handle_commit_txn}},
        {messages::session_state_t::TXN_COMMITTING, messages::session_event_t::DECIDE_TXN_COMMIT, {messages::session_state_t::CONNECTED, handle_decide_txn}},
        {messages::session_state_t::TXN_COMMITTING, messages::session_event_t::DECIDE_TXN_ABORT, {messages::session_state_t::CONNECTED, handle_decide_txn}},
        {messages::session_state_t::ANY, messages::session_event_t::SERVER_SHUTDOWN, {messages::session_state_t::DISCONNECTED, handle_server_shutdown}},
        {messages::session_state_t::ANY, messages::session_event_t::REQUEST_STREAM, {messages::session_state_t::ANY, handle_request_stream}},
        {messages::session_state_t::ANY, messages::session_event_t::REQUEST_MEMORY, {messages::session_state_t::ANY, handle_request_memory}},
    };

    static void apply_transition(messages::session_event_t event, const void* event_data, int* fds, size_t fd_count);

    static void build_server_reply(
        flatbuffers::FlatBufferBuilder& builder,
        messages::session_event_t event,
        messages::session_state_t old_state,
        messages::session_state_t new_state,
        gaia_txn_id_t txn_id = 0,
        size_t log_fd_count = 0,
        gaia::db::memory_manager::address_offset_t object_address_offset = 0);

    static void clear_shared_memory();

    static gaia::db::memory_manager::address_offset_t allocate_from_memory_manager(
        size_t memory_request_size_bytes);

    static void init_memory_manager();

    static void free_uncommitted_allocations(messages::session_event_t txn_status);

    static void init_shared_memory();

    static void request_memory();

    static void init_txn_info();

    static void recover_db();

    static sigset_t mask_signals();

    static void signal_handler(sigset_t sigset, int& signum);

    static void init_listening_socket();

    static bool authenticate_client_socket(int socket);

    static void client_dispatch_handler();

    static void session_handler(int session_socket);

    static std::pair<int, int> get_stream_socket_pair();

    template <typename element_type>
    static void stream_producer_handler(int stream_socket, int cancel_eventfd, std::function<std::optional<element_type>()> generator_fn);

    template <typename element_type>
    static void start_stream_producer(int stream_socket, std::function<std::optional<element_type>()> generator_fn);

    static std::function<std::optional<common::gaia_id_t>()>
    get_id_generator_for_type(common::gaia_type_t type);

    static void get_txn_log_fds_for_snapshot(gaia_txn_id_t begin_ts, std::vector<int>& txn_log_fds);

    static gaia_txn_id_t txn_begin();

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

    static bool invalidate_unknown_ts(gaia_txn_id_t ts);

    static bool is_unknown_ts(gaia_txn_id_t ts);

    static bool is_invalid_ts(gaia_txn_id_t ts);

    static bool is_txn_entry_begin_ts(ts_entry_t ts_entry);

    static bool is_begin_ts(gaia_txn_id_t ts);

    static bool is_txn_entry_commit_ts(ts_entry_t ts_entry);

    static bool is_commit_ts(gaia_txn_id_t ts);

    static bool is_txn_entry_submitted(ts_entry_t ts_entry);

    static bool is_txn_submitted(gaia_txn_id_t begin_ts);

    static bool is_txn_entry_validating(ts_entry_t ts_entry);

    static bool is_txn_validating(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_decided(ts_entry_t ts_entry);

    static bool is_txn_decided(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_committed(ts_entry_t ts_entry);

    static bool is_txn_committed(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_aborted(ts_entry_t ts_entry);

    static bool is_txn_aborted(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_gc_complete(ts_entry_t ts_entry);

    static bool is_txn_gc_complete(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_durable(ts_entry_t ts_entry);

    static bool is_txn_durable(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_active(ts_entry_t ts_entry);

    static bool is_txn_active(gaia_txn_id_t begin_ts);

    static bool is_txn_entry_terminated(ts_entry_t ts_entry);

    static bool is_txn_terminated(gaia_txn_id_t begin_ts);

    static uint64_t get_status(gaia_txn_id_t ts);

    static uint64_t get_status_from_entry(ts_entry_t ts_entry);

    static gaia_txn_id_t get_begin_ts(gaia_txn_id_t commit_ts);

    static gaia_txn_id_t get_commit_ts(gaia_txn_id_t begin_ts);

    static int get_txn_log_fd(gaia_txn_id_t commit_ts);

    static int get_txn_log_fd_from_entry(ts_entry_t commit_ts_entry);

    static bool invalidate_txn_log_fd(gaia_txn_id_t commit_ts);

    static gaia_txn_id_t register_commit_ts(gaia_txn_id_t begin_ts, int log_fd);

    static void set_active_txn_submitted(gaia_txn_id_t begin_ts, gaia_txn_id_t commit_ts);

    static void set_active_txn_terminated(gaia_txn_id_t begin_ts);

    static gaia_txn_id_t submit_txn(gaia_txn_id_t begin_ts, int log_fd);

    static void update_txn_decision(gaia_txn_id_t commit_ts, bool committed);

    static bool txn_logs_conflict(int log_fd1, int log_fd2);

    static bool validate_txn(gaia_txn_id_t commit_ts);

    static void validate_txns_in_range(gaia_txn_id_t start_ts, gaia_txn_id_t end_ts);

    static bool advance_last_applied_txn_commit_ts(gaia_txn_id_t commit_ts);

    static void apply_txn_log_from_ts(gaia_txn_id_t commit_ts);

    static bool set_txn_gc_complete(gaia_txn_id_t commit_ts);

    static void gc_txn_log_from_fd(int log_fd, bool committed = true);

    static void deallocate_txn_log(txn_log_t* txn_log, bool deallocate_new_offsets = false);

    static void dump_ts_entry(gaia_txn_id_t ts);

    static const char* status_to_str(ts_entry_t ts_entry);

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
    // entry even while it is concurrently invalidated (i.e. the fd is closed and
    // its embedded value set to -1). The constructor throws an invalid_log_fd
    // exception if the fd is invalidated during construction.
    class safe_fd_from_ts
    {
    public:
        explicit safe_fd_from_ts(gaia_txn_id_t commit_ts, bool auto_close_fd = true)
            : m_auto_close_fd(auto_close_fd)
        {
            common::retail_assert(is_commit_ts(commit_ts), "You must initialize safe_fd_from_ts from a valid commit_ts!");
            // If the log fd was invalidated, it is either closed or soon will
            // be closed, and therefore we cannot use it. We return early if the
            // fd has already been invalidated to avoid the dup(2) call.
            int log_fd = get_txn_log_fd(commit_ts);
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
                    common::retail_assert(get_txn_log_fd(commit_ts) == -1, "log fd was closed without being invalidated!");
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
            if (get_txn_log_fd(commit_ts) == -1)
            {
                // If we got here, we must have a valid dup fd.
                common::retail_assert(
                    common::is_fd_valid(m_local_log_fd),
                    "fd should be valid if dup() succeeded!");
                // We need to close the duplicated fd since the original fd
                // might have been reused and we would leak it otherwise
                // (because the destructor isn't called if the constructor
                // throws).
                common::close_fd(m_local_log_fd);
                throw invalid_log_fd(commit_ts);
            }
        }

        // The "rule of 3" doesn't apply here since we never pass this object
        // directly to a function; we always extract the safe fd value first.
        safe_fd_from_ts() = delete;
        safe_fd_from_ts(const safe_fd_from_ts&) = delete;
        safe_fd_from_ts& operator=(const safe_fd_from_ts&) = delete;

        ~safe_fd_from_ts()
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

        // This is the only public API. Callers are expected to call this method on
        // a stack-constructed instance of the class before they pass an fd to a
        // function that does not expect it to be concurrently closed.
        int get_fd() const
        {
            return m_local_log_fd;
        }

    private:
        int m_local_log_fd{-1};
        bool m_auto_close_fd{true};
    };
};

} // namespace db
} // namespace gaia
