/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <limits>

namespace gaia
{
namespace db
{

struct txn_metadata_t
{
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
    inline bool is_aborted() const;
    inline bool is_gc_complete() const;
    inline bool is_durable() const;
    inline bool is_active() const;
    inline bool is_terminated() const;

    inline void invalidate_txn_log_fd();
    inline void set_terminated();
    inline void set_gc_complete();

    inline uint64_t get_status() const;

    inline uint64_t get_timestamp() const;

    inline int get_txn_log_fd() const;

    const char* status_to_str() const;

    uint64_t value;
};

#include "txn_metadata.inc"

} // namespace db
} // namespace gaia
