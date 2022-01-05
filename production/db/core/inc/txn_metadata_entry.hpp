/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <bitset>
#include <limits>
#include <sstream>
#include <string>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{
namespace transactions
{

// This value class represents an immutable snapshot of txn metadata for a
// single timestamp, but with no association to a particular timestamp. (The
// association of a metadata entry value with a timestamp is the responsibility
// of the txn_metadata_t class.) This class implements operations (e.g.,
// bitfield manipulations) on the metadata format, so no other classes need
// knowledge of the format. It never reads or writes directly to the txn
// metadata array; that is the responsibility of the txn_metadata_t class.
class txn_metadata_entry_t
{
public:
    inline explicit txn_metadata_entry_t(uint64_t word)
        : m_word(word)
    {
    }

    txn_metadata_entry_t(const txn_metadata_entry_t&) = default;

    // The copy assignment operator is implicitly deleted because this class has
    // no non-static, non-const members, but we make it explicit.
    txn_metadata_entry_t& operator=(const txn_metadata_entry_t&) = delete;

    friend inline bool operator==(txn_metadata_entry_t a, txn_metadata_entry_t b);
    friend inline bool operator!=(txn_metadata_entry_t a, txn_metadata_entry_t b);

private:
    const uint64_t m_word;

public:
    inline uint64_t get_word();

    static inline void check_ts_size(gaia_txn_id_t ts);
    static inline constexpr size_t get_max_ts_count();

    static inline txn_metadata_entry_t uninitialized_value();
    static inline txn_metadata_entry_t sealed_value();
    static inline txn_metadata_entry_t new_begin_ts_entry();
    static inline txn_metadata_entry_t new_commit_ts_entry(gaia_txn_id_t begin_ts, int log_fd);

    inline bool is_uninitialized();
    inline bool is_sealed();
    inline bool is_begin_ts_entry();
    inline bool is_commit_ts_entry();
    inline bool is_submitted();
    inline bool is_validating();
    inline bool is_decided();
    inline bool is_committed();
    inline bool is_aborted();
    inline bool is_gc_complete();
    inline bool is_durable();
    inline bool is_active();
    inline bool is_terminated();

    inline uint64_t get_status();
    inline gaia_txn_id_t get_timestamp();
    inline int get_log_fd();

    inline txn_metadata_entry_t invalidate_log_fd();
    inline txn_metadata_entry_t set_submitted(gaia_txn_id_t commit_ts);
    inline txn_metadata_entry_t set_terminated();
    inline txn_metadata_entry_t set_decision(bool is_committed);
    inline txn_metadata_entry_t set_durable();
    inline txn_metadata_entry_t set_gc_complete();

    inline const char* status_to_str();
    inline std::string dump_metadata();

private:
    // Transaction metadata constants.
    //
    // Transaction metadata format:
    // 64 bits:
    //   0-41 = linked timestamp
    //   42-57 = log fd
    //   58 = reserved
    //   59 = persistence status
    //   60 = gc status
    //   61-63 = txn status
    //
    // txn_status (3) | gc_status (1) | persistence_status (1) | reserved (1) | log_fd (16) | linked_timestamp (42)

    static constexpr uint64_t c_txn_metadata_bits{64ULL};

    // Because we restrict all fds to 16 bits, this is the largest possible
    // value in that range, which we reserve to indicate an invalidated fd
    // (i.e., one which was claimed for deallocation by a maintenance thread).
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
    // These only apply to a commit_ts metadata entry.
    // We don't need TXN_GC_ELIGIBLE or TXN_GC_INITIATED flags, because any txn
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
    // been marked durable (because they might be concurrently read for
    // persistence to the write-ahead log). If persistence is disabled, this
    // flag is unused.
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
    // metadata format.
    static constexpr uint64_t c_txn_reserved_flags_bits{1ULL};

    // Txn log fd embedded in the txn metadata.
    // This is only present in a commit_ts metadata entry.
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

    // Linked txn timestamp embedded in a txn metadata entry. For a commit_ts
    // entry, this is its associated begin_ts, and for a begin_ts entry, this is
    // its associated commit_ts. A commit_ts entry always contains its linked
    // begin_ts, but a begin_ts entry may not be updated with its linked
    // commit_ts until after the associated commit_ts entry has been created.
    //
    // REVIEW: We could save at least 10 bits (conservatively) if we replaced
    // the linked timestamp with its delta from the metadata entry's timestamp (the
    // delta for a linked begin_ts would be implicitly negative). 32 bits should
    // be more than we would ever need, and 16 bits could suffice if we enforced
    // limits on the "age" of an active txn (e.g., if we aborted txns whose
    // begin_ts was > 2^16 older than the last allocated timestamp). We might
    // want to enforce an age limit anyway, of course, to avoid unbounded
    // garbage accumulation (which consumes memory, open fds, and other
    // resources, and also increases txn begin latency, although it doesn't
    // affect read/write latency, because we don't use version chains).

#if __has_feature(thread_sanitizer)
    // We use 32-bit timestamps in TSan builds (and therefore 32GB rather than
    // 32TB for the txn metadata segment), because TSan can't handle huge VM
    // reservations. Per the above estimates, this would allow the server to run
    // for 2-3 days at 10K TPS. This shouldn't be a concern for TSan builds,
    // which are not intended for production.
    //
    // REVIEW (GAIAPLAT-1577): We should be able to revert this restriction when
    // we move the txn metadata to a fixed-size circular buffer.
    static constexpr uint64_t c_txn_ts_bits{32ULL};
#else
    static constexpr uint64_t c_txn_ts_bits{42ULL};
#endif
    static constexpr uint64_t c_txn_ts_shift{0ULL};
    static constexpr uint64_t c_txn_ts_mask{((1ULL << c_txn_ts_bits) - 1) << c_txn_ts_shift};

    // Transaction metadata special values.

    // The first 3 bits of this value are unused for any txn state.
    static constexpr uint64_t c_value_uninitialized{0ULL};

    // The first 3 bits of this value do not correspond to any valid txn status value.
    static constexpr uint64_t c_value_sealed{0b101ULL << c_txn_status_flags_shift};
};

#include "txn_metadata_entry.inc"

} // namespace transactions
} // namespace db
} // namespace gaia
