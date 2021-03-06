/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <atomic>
#include <limits>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{

class txn_metadata_t
{
private:
    // This is an effectively infinite array of timestamp entries, indexed by
    // the txn timestamp counter and containing metadata for every txn that has
    // been submitted to the system.
    //
    // Entries may be "uninitialized", "sealed" (initialized with a
    // special "junk" value and forbidden to be used afterward), or initialized
    // with txn metadata, consisting of 3 status bits, 1 bit for GC status
    // (unknown or complete), 1 bit for persistence status (unknown or
    // complete), 1 bit reserved for future use, 16 bits for a txn log fd, and
    // 42 bits for a linked timestamp (i.e., the commit timestamp of a submitted
    // txn embedded in its begin timestamp metadata, or the begin timestamp of a
    // submitted txn embedded in its commit timestamp metadata). The 3 status bits
    // use the high bit to distinguish begin timestamps from commit timestamps,
    // and 2 bits to store the state of an active, terminated, or submitted txn.
    //
    // The array is always accessed without any locking, but its entries have
    // read and write barriers (via std::atomic) that ensure causal consistency
    // between any threads that read or write the same txn metadata. Any writes to
    // entries that may be written by multiple threads use CAS operations.
    //
    // The array's memory is managed via mmap(MAP_NORESERVE). We reserve 32TB of
    // virtual address space (1/8 of the total virtual address space available
    // to the process), but allocate physical pages only on first access. When a
    // range of timestamp entries falls behind the watermark, its physical pages
    // can be deallocated via madvise(MADV_FREE).
    //
    // REVIEW: Since we reserve 2^45 bytes of virtual address space and each
    // array entry is 8 bytes, we can address the whole range using 2^42
    // timestamps. If we allocate 2^10 timestamps/second, we will use up all our
    // timestamps in 2^32 seconds, or about 2^7 years. If we allocate 2^20
    // timestamps/second, we will use up all our timestamps in 2^22 seconds, or
    // about a month and a half. If this is an issue, then we could treat the
    // array as a circular buffer, using a separate wraparound counter to
    // calculate the array offset from a timestamp, and we can use the 3
    // reserved bits in the txn metadata to extend our range by a factor of
    // 8, so we could allocate 2^20 timestamps/second for a full year. If we
    // need a still larger timestamp range (say 64-bit timestamps, with
    // wraparound), we could just store the difference between a commit
    // timestamp and its txn's begin timestamp, which should be possible to
    // bound to no more than half the bits we use for the full timestamp, so we
    // would still need only 32 bits for a timestamp reference in the timestamp
    // metadata. (We could store the array offset instead, but that would be
    // dangerous when we approach wraparound.)
    static inline std::atomic<txn_metadata_t>* s_txn_metadata_map = nullptr;

    static inline void check_ts_size(gaia_txn_id_t ts);

public:
    static void init_txn_metadata_map();

    static inline bool is_uninitialized_ts(gaia_txn_id_t ts);
    // static inline bool is_sealed_ts(gaia_txn_id_t ts);
    static inline bool is_begin_ts(gaia_txn_id_t ts);
    static inline bool is_commit_ts(gaia_txn_id_t ts);
    static inline bool is_txn_submitted(gaia_txn_id_t begin_ts);
    static inline bool is_txn_validating(gaia_txn_id_t commit_ts);
    static inline bool is_txn_decided(gaia_txn_id_t commit_ts);
    static inline bool is_txn_committed(gaia_txn_id_t commit_ts);
    // static inline bool is_txn_aborted(gaia_txn_id_t commit_ts);
    static inline bool is_txn_gc_complete(gaia_txn_id_t commit_ts);
    static inline bool is_txn_durable(gaia_txn_id_t commit_ts);
    static inline bool is_txn_active(gaia_txn_id_t begin_ts);
    // static inline bool is_txn_terminated(gaia_txn_id_t begin_ts);

    static inline gaia_txn_id_t get_begin_ts(gaia_txn_id_t commit_ts);
    static inline gaia_txn_id_t get_commit_ts(gaia_txn_id_t begin_ts);

    // static inline uint64_t get_status(gaia_txn_id_t ts);
    static inline int get_txn_log_fd(gaia_txn_id_t commit_ts);

    static bool seal_uninitialized_ts(gaia_txn_id_t ts);
    static bool invalidate_txn_log_fd(gaia_txn_id_t commit_ts);
    static void set_active_txn_submitted(gaia_txn_id_t begin_ts, gaia_txn_id_t commit_ts);
    static void set_active_txn_terminated(gaia_txn_id_t begin_ts);
    static void update_txn_decision(gaia_txn_id_t commit_ts, bool is_committed);
    static bool set_txn_gc_complete(gaia_txn_id_t commit_ts);

    static gaia_txn_id_t txn_begin();
    static gaia_txn_id_t register_commit_ts(gaia_txn_id_t begin_ts, int log_fd);

    static void dump_txn_metadata(gaia_txn_id_t ts);

private:
    // Transaction metadata constants.
    //
    // Transaction metadata format:
    // 64 bits: txn_status (3) | gc_status (1) | persistence_status (1) | reserved (1) | log_fd (16) | linked_timestamp (42)
    static constexpr uint64_t c_txn_metadata_bits{64ULL};

    // Since we restrict all fds to 16 bits, this is the largest possible value
    // in that range, which we reserve to indicate an invalidated fd (i.e., one
    // which was claimed for deallocation by a maintenance thread).
    static constexpr uint16_t c_invalid_txn_log_fd_bits{std::numeric_limits<uint16_t>::max()};

    // Transaction status flags.
    static constexpr uint64_t c_txn_status_flags_bits{3ULL};
    static constexpr uint64_t c_txn_status_flags_shift{c_txn_metadata_bits - c_txn_status_flags_bits};
    static constexpr uint64_t c_txn_status_flags_mask{
        ((1ULL << c_txn_status_flags_bits) - 1) << c_txn_status_flags_shift};

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
    // These only apply to a commit_ts metadata.
    // We don't need TXN_GC_ELIGIBLE or TXN_GC_INITIATED flags, since any txn
    // behind the post-apply watermark (and with TXN_PERSISTENCE_COMPLETE set if
    // persistence is enabled) is eligible for GC, and an invalidated log fd
    // indicates that GC is in progress.
    static constexpr uint64_t c_txn_gc_flags_bits{1ULL};
    static constexpr uint64_t c_txn_gc_flags_shift{
        (c_txn_metadata_bits - c_txn_gc_flags_bits) - c_txn_status_flags_bits};
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
        (c_txn_metadata_bits - c_txn_persistence_flags_bits)
        - (c_txn_status_flags_bits + c_txn_gc_flags_bits)};
    static constexpr uint64_t c_txn_persistence_flags_mask{
        ((1ULL << c_txn_persistence_flags_bits) - 1) << c_txn_persistence_flags_shift};

    // These are all commit_ts flag values.
    static constexpr uint64_t c_txn_persistence_unknown{0b0ULL};
    static constexpr uint64_t c_txn_persistence_complete{0b1ULL};

    // This is a placeholder for the single (currently) reserved bit in the txn
    // metadata.
    static constexpr uint64_t c_txn_reserved_flags_bits{1ULL};

    // Txn log fd embedded in the txn metadata.
    // This is only present in a commit_ts metadata.
    // NB: we assume that any fd will be < 2^16 - 1!
    static constexpr uint64_t c_txn_log_fd_bits{16ULL};
    static constexpr uint64_t c_txn_log_fd_shift{
        (c_txn_metadata_bits - c_txn_log_fd_bits)
        - (c_txn_status_flags_bits
           + c_txn_gc_flags_bits
           + c_txn_persistence_flags_bits
           + c_txn_reserved_flags_bits)};
    static constexpr uint64_t c_txn_log_fd_mask{
        ((1ULL << c_txn_log_fd_bits) - 1) << c_txn_log_fd_shift};

    // Linked txn timestamp embedded in the txn metadata. For a commit_ts
    // metadata, this is its associated begin_ts, and for a begin_ts metadata, this is
    // its associated commit_ts. A commit_ts metadata always contains its linked
    // begin_ts, but a begin_ts metadata may not be updated with its linked
    // commit_ts until after the associated commit_ts metadata has been created.
    //
    // REVIEW: We could save at least 10 bits (conservatively) if we replaced
    // the linked timestamp with its delta from the metadata's timestamp (the delta
    // for a linked begin_ts would be implicitly negative). 32 bits should be
    // more than we would ever need, and 16 bits could suffice if we enforced
    // limits on the "age" of an active txn (e.g., if we aborted txns whose
    // begin_ts was > 2^16 older than the last allocated timestamp). We might
    // want to enforce an age limit anyway, of course, to avoid unbounded
    // garbage accumulation (which consumes memory, open fds, and other
    // resources, and also increases txn begin latency, although it doesn't
    // affect read/write latency, since we don't use version chains).
    static constexpr uint64_t c_txn_ts_bits{42ULL};
    static constexpr uint64_t c_txn_ts_shift{0ULL};
    static constexpr uint64_t c_txn_ts_mask{((1ULL << c_txn_ts_bits) - 1) << c_txn_ts_shift};

    // Transaction metadata special values.
    // The first 3 bits of this value are unused for any txn state.
    static constexpr uint64_t c_value_uninitialized{0ULL};

    // The first 3 bits of this value do not correspond to any valid txn status value.
    static constexpr uint64_t c_value_sealed{0b101ULL << c_txn_status_flags_shift};

private:
    txn_metadata_t() noexcept;
    explicit txn_metadata_t(uint64_t value);

    inline bool is_uninitialized() const;
    inline bool is_sealed() const;
    inline bool is_begin_ts() const;
    inline bool is_commit_ts() const;
    inline bool is_submitted() const;
    inline bool is_validating() const;
    inline bool is_decided() const;
    inline bool is_committed() const;
    // inline bool is_aborted() const;
    inline bool is_gc_complete() const;
    inline bool is_durable() const;
    inline bool is_active() const;
    // inline bool is_terminated() const;

    inline uint64_t get_status() const;
    inline gaia_txn_id_t get_timestamp() const;
    inline int get_txn_log_fd() const;

    inline void invalidate_txn_log_fd();
    inline void set_terminated();
    inline void set_gc_complete();

    const char* status_to_str() const;

private:
    uint64_t m_value;
};

#include "txn_metadata.inc"

} // namespace db
} // namespace gaia
