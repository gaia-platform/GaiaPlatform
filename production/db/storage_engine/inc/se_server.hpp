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
#include "fd_helpers.hpp"
#include "memory_manager.hpp"
#include "messages_generated.h"
#include "persistent_store_manager.hpp"
#include "se_types.hpp"
#include "stack_allocator.hpp"

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
    friend gaia::db::memory_manager::address_offset_t gaia::db::allocate_object(
        gaia_locator_t locator,
        gaia::db::memory_manager::address_offset_t old_slot_offset,
        size_t size);

public:
    static void run(bool disable_persistence = false);
    static constexpr char c_disable_persistence_flag[] = "--disable-persistence";
    static void register_object_deallocator(std::function<void(gaia_offset_t)>);

private:
    // from https://www.man7.org/linux/man-pages/man2/eventfd.2.html
    static constexpr uint64_t c_max_semaphore_count = std::numeric_limits<uint64_t>::max() - 1;
    // This is arbitrary but seems like a reasonable starting point (pending benchmarks).
    static constexpr size_t c_stream_batch_size{1ULL << 10};
    static inline int s_server_shutdown_eventfd = -1;
    static inline int s_listening_socket = -1;
    static inline int s_fd_data = -1;
    static inline data* s_data = nullptr;
    static inline int s_fd_locators = -1;
    static inline locators* s_shared_locators = nullptr;
    thread_local static inline log* s_log = nullptr;
    thread_local static inline int s_fd_log = -1;
    thread_local static inline gaia_txn_id_t s_txn_id = c_invalid_gaia_txn_id;
    static inline std::unique_ptr<persistent_store_manager> rdb{};
    thread_local static inline int s_session_socket = -1;
    thread_local static inline session_state_t s_session_state = session_state_t::DISCONNECTED;
    thread_local static inline bool s_session_shutdown = false;
    thread_local static inline int s_session_shutdown_eventfd = -1;
    thread_local static inline std::vector<std::thread> s_session_owned_threads{};
    static inline bool s_disable_persistence = false;
    static inline std::unique_ptr<gaia::db::memory_manager::memory_manager_t> memory_manager{};

    // Keeps track of stack allocators belonging to the current transaction executing on this thread.
    // On commit/rollback, all stack allocators belonging to a transaction are removed from this list.
    // In case of receiving any of the following epoll events - [EPOLLRDHUP, EPOLLHUP, EPOLLERR] on the server_client socket fd
    // all unused/uncommitted stack allocators in this list will be purged before terminating the connection.
    thread_local static inline std::vector<std::unique_ptr<gaia::db::memory_manager::stack_allocator_t>> s_active_stack_allocators{};

    // function pointer type that executes side effects of a state transition
    // This is an "endless" array of timestamp entries, indexed by the txn
    // timestamp counter and containing all information for every txn that has
    // been submitted to the system. Entries may be "unknown" (uninitialized),
    // "invalid" (initialized with a special "junk" value and forbidden to be
    // used afterward), or initialized with txn information, consisting of 3
    // status bits, 16 bits for a txn log fd, 3 reserved bits, and 42 bits for a
    // timestamp reference (the begin timestamp of a submitted txn that
    // allocated a commit timestamp, or the commit timestamp of a submitted txn
    // that allocated a begin timestamp). The 3 status bits use the high bit to
    // distinguish begin timestamps from commit timestamps, and 2 bits to store
    // the state of an active or submitted txn. The array is always accessed
    // without any locking, but its entries have read and write barriers (via
    // std::atomic) that ensure a happens-before relationship between any
    // threads that read or write the same entry. Any writes to entries that may
    // be written by multiple threads use CAS operations (via
    // std::atomic::compare_exchange_strong).
    //
    // The array's memory is managed via mmap(MAP_NORESERVE). We reserve 32TB of
    // virtual address space (1/8 of the total virtual address space available
    // to the process), but allocate physical pages only on first access. When a
    // range of timestamp entries falls behind the watermark, its physical pages
    // can be deallocated via madvise(MADV_FREE). Since we reserve 2^45 bytes of
    // virtual address space and each array entry is 8 bytes, we can address the
    // whole range using 2^42 timestamps. If we allocate 2^10 timestamps/second,
    // we will use up all our timestamps in 2^32 seconds, or about 2^7 years. If
    // we allocate 2^20 timestamps/second, we will use up all our timestamps in
    // 2^22 seconds, or about a month and a half. If this is an issue, then we
    // could treat the array as a circular buffer, using a separate wraparound
    // counter to calculate the array offset from a timestamp, and we can use
    // the 3 reserved bits in the timestamp entry to extend our range by a
    // factor of 8, so we could allocate 2^20 timestamps/second for a full year.
    // If we need a still larger timestamp range (say 64-bit timestamps, with
    // wraparound), we could just store the difference between a commit
    // timestamp and its txn's begin timestamp, which should be possible to
    // bound to no more than half the bits we use for the full timestamp, so we
    // would still need only 32 bits for a timestamp reference in the timestamp
    // entry. (We could store the array offset instead, but that would be
    // dangerous when we approach wraparound.)
    static inline std::atomic<uint64_t>* s_txn_info = nullptr;
    // This marks a boundary at or before which every committed commit_ts may be
    // assumed to have been applied to the shared locator view, and after which
    // no commit_ts may be assumed to have had its txn log invalidated or freed
    // (even if it aborted). No undecided commit_ts may precede the boundary,
    // nor may any active or validating begin_ts. Since the shared view is
    // updated non-atomically, there may be a commit_ts later than the boundary
    // which is partially or fully applied to the shared locator view. But since
    // log replay is idempotent, we can safely replay logs that have already
    // been partially or fully applied to the shared view, starting with the
    // last commit_ts at or before the boundary. We always apply a full prefix
    // of committed txns to the shared view; there can be no gaps in the
    // sequence of committed txns that have been applied to the view (since no
    // commit_ts before the boundary can be undecided).
    static inline std::atomic<gaia_txn_id_t> s_last_applied_commit_ts_upper_bound = c_invalid_gaia_txn_id;
    // This is an extension point called by the transactional system when the
    // "watermark" advances (i.e., the oldest active txn terminates or commits),
    // allowing all the superseded object versions in txns with commit
    // timestamps before the watermark to be freed (since they are no longer
    // visible to any present or future txns).
    static inline std::function<void(gaia_offset_t)> s_object_deallocator_fn{};

    // Transaction timestamp entry constants.
    static constexpr uint64_t c_txn_status_entry_bits{64ULL};
    static constexpr uint64_t c_txn_status_flags_bits{3ULL};
    static constexpr uint64_t c_txn_status_flags_shift{c_txn_status_entry_bits - c_txn_status_flags_bits};
    static constexpr uint64_t c_txn_status_flags_mask{
        ((1ULL << c_txn_status_flags_bits) - 1) << c_txn_status_flags_shift};
    static constexpr uint64_t c_txn_log_fd_bits{16ULL};
    static constexpr uint64_t c_txn_log_fd_shift{
        (c_txn_status_entry_bits - c_txn_log_fd_bits) - c_txn_status_flags_bits};
    static constexpr uint64_t c_txn_log_fd_mask{
        ((1ULL << c_txn_log_fd_bits) - 1) << c_txn_log_fd_shift};
    static constexpr uint64_t c_txn_reserved_bits{3ULL};
    static constexpr uint64_t c_txn_ts_bits{42ULL};
    static constexpr uint64_t c_txn_ts_mask{(1ULL << c_txn_ts_bits) - 1};

    // Transaction status flags.
    static constexpr uint64_t c_txn_status_active{0b010ULL};
    static constexpr uint64_t c_txn_status_submitted{0b011ULL};
    static constexpr uint64_t c_txn_status_terminated{0b001ULL};
    // This is a mask, not a value.
    static constexpr uint64_t c_txn_status_commit_ts{0b100ULL};
    static constexpr uint64_t c_txn_status_commit_mask{
        c_txn_status_commit_ts << c_txn_status_flags_shift};
    static constexpr uint64_t c_txn_status_validating{0b100ULL};
    // This is a mask, not a value.
    static constexpr uint64_t c_txn_status_decided{0b110ULL};
    static constexpr uint64_t c_txn_status_decided_mask{
        c_txn_status_decided << c_txn_status_flags_shift};
    static constexpr uint64_t c_txn_status_committed{0b111ULL};
    static constexpr uint64_t c_txn_status_aborted{0b110ULL};

    // Transaction special values.
    // The first 3 bits of this value are unused for any txn state.
    static constexpr uint64_t c_txn_entry_unknown{0ULL};
    // The first 3 bits of this value are unused for any txn state.
    static constexpr uint64_t c_txn_entry_invalid{0b101ULL << c_txn_status_flags_shift};
    // Since we restrict all fds to 16 bits, this is the largest possible value
    // in that range, which we reserve to indicate an invalidated fd (i.e., one
    // which was claimed for deallocation by a maintenance thread).
    static constexpr uint16_t c_invalid_txn_log_fd_bits{std::numeric_limits<uint16_t>::max()};

    // Function pointer type that executes side effects of a state transition.
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
    static void handle_request_memory(int*, size_t, session_event_t, const void*, session_state_t, session_state_t);

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
        {session_state_t::ANY, session_event_t::REQUEST_MEMORY, {session_state_t::ANY, handle_request_memory}},
    };

    static void free_stack_allocators(bool deallocate_stack_allocator);

    static void apply_transition(session_event_t event, const void* event_data, int* fds, size_t fd_count);

    static void build_server_reply(
        FlatBufferBuilder& builder,
        session_event_t event,
        session_state_t old_state,
        session_state_t new_state,
        gaia_txn_id_t txn_id = 0,
        const gaia::db::memory_manager::stack_allocator_t* const new_stack_allocator = nullptr);

    static void clear_shared_memory();

    static gaia::db::memory_manager::stack_allocator_t allocate_stack_allocator(
        size_t txn_memory_request_size_bytes);

    static void init_memory_manager();

    static void get_memory_info_from_request_and_free(
        session_event_t event,
        bool commit_success);

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

    static void fd_stream_producer_handler(int stream_socket, int cancel_eventfd, std::function<std::optional<gaia_txn_id_t>()> ts_generator_fn);

    template <typename element_type>
    static void start_stream_producer(int stream_socket, std::function<std::optional<element_type>()> generator_fn);

    static void start_fd_stream_producer(int stream_socket, std::function<std::optional<gaia_txn_id_t>()> ts_generator_fn);

    static std::function<std::optional<gaia_id_t>()>
    get_id_generator_for_type(gaia_type_t type);

    static std::function<std::optional<gaia_txn_id_t>()>
    get_commit_ts_generator_for_begin_ts(gaia_txn_id_t begin_ts);

    static gaia_txn_id_t txn_begin();

    static void txn_rollback();

    static bool txn_commit();

    static gaia::db::memory_manager::address_offset_t allocate_object(
        gaia_locator_t locator,
        gaia::db::memory_manager::address_offset_t old_slot_offset,
        size_t size);

    static void update_apply_watermark(gaia_txn_id_t);

    static bool advance_watermark_ts(std::atomic<gaia_txn_id_t>& watermark, gaia_txn_id_t ts);

    static bool invalidate_unknown_ts(gaia_txn_id_t ts);

    static bool is_unknown_ts(gaia_txn_id_t ts);

    static bool is_invalid_ts(gaia_txn_id_t ts);

    static bool is_txn_entry_begin_ts(uint64_t ts_entry);

    static bool is_begin_ts(gaia_txn_id_t ts);

    static bool is_txn_entry_commit_ts(uint64_t ts_entry);

    static bool is_commit_ts(gaia_txn_id_t ts);

    static bool is_txn_entry_submitted(uint64_t ts_entry);

    static bool is_txn_submitted(gaia_txn_id_t begin_ts);

    static bool is_txn_entry_validating(uint64_t ts_entry);

    static bool is_txn_validating(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_decided(uint64_t ts_entry);

    static bool is_txn_decided(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_committed(uint64_t ts_entry);

    static bool is_txn_committed(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_aborted(uint64_t ts_entry);

    static bool is_txn_aborted(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_active(uint64_t ts_entry);

    static bool is_txn_active(gaia_txn_id_t begin_ts);

    static bool is_txn_entry_terminated(uint64_t ts_entry);

    static bool is_txn_terminated(gaia_txn_id_t begin_ts);

    static uint64_t get_status(gaia_txn_id_t ts);

    static uint64_t get_status_from_entry(uint64_t ts_entry);

    static gaia_txn_id_t get_begin_ts(gaia_txn_id_t commit_ts);

    static gaia_txn_id_t get_commit_ts(gaia_txn_id_t begin_ts);

    static int get_txn_log_fd(gaia_txn_id_t commit_ts);

    static int get_txn_log_fd_from_entry(uint64_t commit_ts_entry);

    static bool execute_with_log_fd_from_ts(gaia_txn_id_t commit_ts, std::function<void(int log_fd)> fn, bool close_dup_fd_on_exit = true);

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

    static void apply_txn_redo_log_from_ts(gaia_txn_id_t commit_ts);

    static void gc_txn_undo_log(int log_fd);

    static void dump_ts_entry(gaia_txn_id_t ts);

    static const char* status_to_str(uint64_t ts_entry);

    class invalid_log_fd : public gaia_exception
    {
    public:
        invalid_log_fd(gaia_txn_id_t commit_ts)
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
        safe_fd_from_ts(gaia_txn_id_t commit_ts, bool auto_close_fd = true)
            : m_auto_close_fd(auto_close_fd)
        {
            retail_assert(is_commit_ts(commit_ts), "You must initialize safe_fd_from_ts from a valid commit_ts!");
            // If the log fd was invalidated, it is either closed or soon will be
            // closed, and therefore we cannot use it.
            int log_fd = get_txn_log_fd(commit_ts);
            if (log_fd == -1)
            {
                throw invalid_log_fd(commit_ts);
            }

            // To avoid races from the log fd being closed out from under us by a
            // concurrent updater, we first dup() the fd, and then if the dup was
            // successful, test the log fd entry again to ensure we aren't reusing a
            // closed log fd. If the log fd was invalidated, then we need to close our
            // dup fd and return false.
            try
            {
                m_local_log_fd = dup_fd(log_fd);
            }
            catch (const system_error& e)
            {
                // The log fd was already closed by another thread (after being
                // invalidated).
                // NB: This does not handle the case of the log fd being closed and then
                // reused before our dup() call. That case is handled below, where we
                // check for invalidation.
                if (e.get_errno() == EBADF)
                {
                    // The log fd must be invalidated before it is closed.
                    retail_assert(get_txn_log_fd(commit_ts) == -1, "log fd was closed without being invalidated!");
                    // We lost the race because the log fd was invalidated and closed after our check.
                    throw invalid_log_fd(commit_ts);
                }
                else
                {
                    throw;
                }
            }

            // If we were able to duplicate the log fd, check to be sure it wasn't
            // invalidated (and thus possibly closed before dup), so we know we aren't
            // reusing a closed fd.
            if (get_txn_log_fd(commit_ts) == -1)
            {
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
                close_fd(m_local_log_fd);
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
