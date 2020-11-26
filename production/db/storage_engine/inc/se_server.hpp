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
    // This is the "watermark": the begin timestamp of the oldest active txn.
    // When a txn log's commit timestamp is older than the watermark, both the
    // undo versions in the log and the log itself can be safely deallocated.
    static inline std::atomic<gaia_txn_id_t> s_oldest_active_txn_begin_ts = c_invalid_gaia_txn_id;
    // This is the commit timestamp of the latest txn known to have been fully
    // applied to the shared locator view. Since the shared view is updated
    // non-atomically, there may be later txns partially or fully applied to the
    // log. But since log replay is idempotent, we can safely replay logs that
    // have already been partially or fully applied to the shared view. We
    // always apply a full prefix of committed txns to the shared view; there
    // can be no gaps in the sequence of committed txns that have been applied
    // to the view.
    static inline std::atomic<gaia_txn_id_t> s_last_applied_txn_commit_ts = c_invalid_gaia_txn_id;

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

    static void fd_stream_producer_handler(int stream_socket, int cancel_eventfd, std::function<std::optional<int>()> generator_fn);

    template <typename element_type>
    static void start_stream_producer(int stream_socket, std::function<std::optional<element_type>()> generator_fn);

    static void start_fd_stream_producer(int stream_socket, std::function<std::optional<int>()> generator_fn);

    static std::function<std::optional<gaia_id_t>()>
    get_id_generator_for_type(gaia_type_t type);

    static std::function<std::optional<int>()>
    get_log_fd_generator_for_begin_ts(gaia_txn_id_t begin_ts);

    static gaia_txn_id_t txn_begin();

    static void txn_rollback();

    static bool txn_commit();

    static bool invalidate_ts(gaia_txn_id_t ts);

    static bool is_unknown_ts(gaia_txn_id_t ts);

    static bool is_invalid_ts(gaia_txn_id_t ts);

    static bool is_commit_ts(gaia_txn_id_t ts);

    static bool is_txn_entry_submitted(uint64_t ts_entry);

    static bool is_txn_submitted(gaia_txn_id_t begin_ts);

    static bool is_txn_validating(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_decided(uint64_t ts_entry);

    static bool is_txn_decided(gaia_txn_id_t commit_ts);

    static bool is_txn_entry_committed(uint64_t ts_entry);

    static bool is_txn_committed(gaia_txn_id_t commit_ts);

    static bool is_txn_active(gaia_txn_id_t begin_ts);

    static bool is_txn_terminated(gaia_txn_id_t begin_ts);

    static uint64_t get_status(gaia_txn_id_t ts);

    static uint64_t get_status_from_entry(uint64_t ts_entry);

    static gaia_txn_id_t get_begin_ts(gaia_txn_id_t commit_ts);

    static gaia_txn_id_t get_commit_ts(gaia_txn_id_t begin_ts);

    static int get_txn_log_fd(gaia_txn_id_t commit_ts);

    static bool register_txn_log(gaia_txn_id_t begin_ts, gaia_txn_id_t commit_ts, int log_fd);

    static bool set_active_txn_submitted(gaia_txn_id_t begin_ts, gaia_txn_id_t commit_ts);

    static void set_active_txn_terminated(gaia_txn_id_t begin_ts);

    static gaia_txn_id_t submit_txn(gaia_txn_id_t begin_ts, int log_fd);

    static void update_txn_decision(gaia_txn_id_t commit_ts, bool committed);

    static bool txn_logs_conflict(int log_fd1, int log_fd2);

    static bool validate_txn(gaia_txn_id_t begin_ts, gaia_txn_id_t commit_ts, int log_fd, bool in_committing_txn = true);

    static void validate_txns_in_range(gaia_txn_id_t start_ts, gaia_txn_id_t end_ts);

    static void dump_ts_entry(gaia_txn_id_t ts);

    static const char* status_to_str(uint64_t ts_entry);
};

} // namespace db
} // namespace gaia
