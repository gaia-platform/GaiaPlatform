/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <limits>
#include <ostream>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{
namespace transactions
{

// This value class represents an immutable snapshot of txn metadata, with no
// association to a timestamp. The association of a metadata entry value with a
// timestamp is the responsibility of the txn_metadata_t class.
class txn_metadata_entry_t
{
private:
    const uint64_t m_word;

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

    // Because we restrict all fds to 16 bits, this is the largest possible value
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
    // affect read/write latency, because we don't use version chains).
    static constexpr uint64_t c_txn_ts_bits{42ULL};
    static constexpr uint64_t c_txn_ts_shift{0ULL};
    static constexpr uint64_t c_txn_ts_mask{((1ULL << c_txn_ts_bits) - 1) << c_txn_ts_shift};

    // Transaction metadata special values.

    // The first 3 bits of this value are unused for any txn state.
    static constexpr uint64_t c_value_uninitialized{0ULL};

    // The first 3 bits of this value do not correspond to any valid txn status value.
    static constexpr uint64_t c_value_sealed{0b101ULL << c_txn_status_flags_shift};

public:
    inline explicit txn_metadata_entry_t(uint64_t word)
        : m_word(word)
    {
    }

    txn_metadata_entry_t(const txn_metadata_entry_t&) = default;

    // The copy assignment operator is implicitly deleted because this class has
    // no non-static, non-const members, but we make it explicit.
    txn_metadata_entry_t& operator=(const txn_metadata_entry_t&) = delete;

    friend inline bool operator==(txn_metadata_entry_t a, txn_metadata_entry_t b)
    {
        return (a.m_word == b.m_word);
    }

    friend inline bool operator!=(txn_metadata_entry_t a, txn_metadata_entry_t b)
    {
        return (a.m_word != b.m_word);
    }

    inline uint64_t get_word()
    {
        return m_word;
    }

    static inline void check_ts_size(gaia_txn_id_t ts)
    {
        ASSERT_PRECONDITION(
            ts < (1ULL << c_txn_ts_bits),
            "Timestamp values must fit in 42 bits!");
    }

    static inline constexpr size_t get_max_ts_count()
    {
        return (1ULL << c_txn_ts_bits);
    }

    static inline txn_metadata_entry_t uninitialized_value()
    {
        return txn_metadata_entry_t{c_value_uninitialized};
    }

    static inline txn_metadata_entry_t sealed_value()
    {
        return txn_metadata_entry_t{c_value_sealed};
    }

    static inline txn_metadata_entry_t new_begin_ts_entry()
    {
        // Any initial begin_ts metadata entry must have its status initialized to TXN_ACTIVE.
        // All other bits should be 0.
        return txn_metadata_entry_t{c_txn_status_active << c_txn_status_flags_shift};
    }

    static inline txn_metadata_entry_t new_commit_ts_entry(gaia_txn_id_t begin_ts, int log_fd)
    {
        // We construct the commit_ts metadata by concatenating required bits.
        uint64_t shifted_log_fd = static_cast<uint64_t>(log_fd) << c_txn_log_fd_shift;
        constexpr uint64_t c_shifted_status_flags{
            c_txn_status_validating << c_txn_status_flags_shift};
        return txn_metadata_entry_t{c_shifted_status_flags | shifted_log_fd | begin_ts};
    }

    inline bool is_uninitialized()
    {
        return (m_word == c_value_uninitialized);
    }

    inline bool is_sealed()
    {
        return (m_word == c_value_sealed);
    }

    inline bool is_begin_ts_entry()
    {
        // The "uninitialized" value has the commit bit unset, so we need to check it as well.
        return (!is_uninitialized() && ((m_word & c_txn_status_commit_mask) == 0));
    }

    inline bool is_commit_ts_entry()
    {
        // The "sealed" value has the commit bit set, so we need to check it as well.
        return (!is_sealed() && ((m_word & c_txn_status_commit_mask) == c_txn_status_commit_mask));
    }

    inline bool is_submitted()
    {
        return (get_status() == c_txn_status_submitted);
    }

    inline bool is_validating()
    {
        return (get_status() == c_txn_status_validating);
    }

    inline bool is_decided()
    {
        constexpr uint64_t c_decided_mask = c_txn_status_decided << c_txn_status_flags_shift;
        return ((m_word & c_decided_mask) == c_decided_mask);
    }

    inline bool is_committed()
    {
        return (get_status() == c_txn_status_committed);
    }

    inline bool is_aborted()
    {
        return (get_status() == c_txn_status_aborted);
    }

    inline bool is_gc_complete()
    {
        uint64_t gc_flags = (m_word & c_txn_gc_flags_mask) >> c_txn_gc_flags_shift;
        return (gc_flags == c_txn_gc_complete);
    }

    inline bool is_durable()
    {
        uint64_t persistence_flags = (m_word & c_txn_persistence_flags_mask) >> c_txn_persistence_flags_shift;
        return (persistence_flags == c_txn_persistence_complete);
    }

    inline bool is_active()
    {
        return (get_status() == c_txn_status_active);
    }

    inline bool is_terminated()
    {
        return (get_status() == c_txn_status_terminated);
    }

    inline uint64_t get_status()
    {
        return ((m_word & c_txn_status_flags_mask) >> c_txn_status_flags_shift);
    }

    inline gaia_txn_id_t get_timestamp()
    {
        // The timestamp is in the low 42 bits of the txn metadata.
        return (m_word & c_txn_ts_mask);
    }

    inline int get_log_fd()
    {
        ASSERT_PRECONDITION(is_commit_ts_entry(), "Not a commit timestamp!");

        // The txn log fd is the 16 bits of the ts metadata after the 3 status bits.
        uint16_t fd = (m_word & c_txn_log_fd_mask) >> c_txn_log_fd_shift;

        // If the log fd is invalidated, then return an invalid fd value (-1).
        return (fd != c_invalid_txn_log_fd_bits) ? static_cast<int>(fd) : -1;
    }

    inline txn_metadata_entry_t invalidate_log_fd()
    {
        ASSERT_PRECONDITION(is_commit_ts_entry(), "Not a commit timestamp!");
        ASSERT_PRECONDITION(is_decided(), "Cannot invalidate an undecided txn!");

        return txn_metadata_entry_t{m_word | c_txn_log_fd_mask};
    }

    inline txn_metadata_entry_t set_submitted(gaia_txn_id_t commit_ts)
    {
        ASSERT_PRECONDITION(is_active(), "Not an active transaction!");

        // Transition the begin_ts metadata to the TXN_SUBMITTED state.
        constexpr uint64_t c_submitted_flags
            = c_txn_status_submitted << c_txn_status_flags_shift;

        // A submitted begin_ts metadata has the commit_ts in its low bits.
        return txn_metadata_entry_t{c_submitted_flags | commit_ts};
    }

    inline txn_metadata_entry_t set_terminated()
    {
        ASSERT_PRECONDITION(is_active(), "Not an active transaction!");

        constexpr uint64_t c_terminated_flags = c_txn_status_terminated << c_txn_status_flags_shift;
        return txn_metadata_entry_t{
            c_terminated_flags | (m_word & ~c_txn_status_flags_mask)};
    }

    inline txn_metadata_entry_t set_decision(bool is_committed)
    {
        // The commit_ts metadata must be in state TXN_VALIDATING or TXN_DECIDED.
        // We allow the latter to enable idempotent concurrent validation.
        ASSERT_PRECONDITION(
            is_validating() || is_decided(),
            "commit_ts metadata must be in validating or decided state!");

        uint64_t decided_status_flags{is_committed ? c_txn_status_committed : c_txn_status_aborted};

        // This masks out all bits except the commit_ts flag bits.
        constexpr uint64_t c_commit_flags_mask = ~c_txn_status_commit_ts << c_txn_status_flags_shift;

        // Turn off all commit flag bits, then set the decision flags.
        return txn_metadata_entry_t{
            (m_word & ~c_commit_flags_mask) | (decided_status_flags << c_txn_status_flags_shift)};
    }

    inline txn_metadata_entry_t set_durable()
    {
        ASSERT_PRECONDITION(is_decided(), "Not a decided transaction!");

        // Set persistence status to TXN_DURABLE.
        return txn_metadata_entry_t{
            m_word | (c_txn_persistence_complete << c_txn_persistence_flags_shift)};
    }

    inline txn_metadata_entry_t set_gc_complete()
    {
        ASSERT_PRECONDITION(is_decided(), "Not a decided transaction!");

        // Set GC status to TXN_GC_COMPLETE.
        return txn_metadata_entry_t{
            m_word | (c_txn_gc_complete << c_txn_gc_flags_shift)};
    }

    inline const char* status_to_str()
    {
        ASSERT_PRECONDITION(
            !is_uninitialized() && !is_sealed(),
            "Not a valid txn metadata entry!");

        uint64_t status = get_status();
        switch (status)
        {
        case c_txn_status_active:
            return "ACTIVE";
        case c_txn_status_submitted:
            return "SUBMITTED";
        case c_txn_status_terminated:
            return "TERMINATED";
        case c_txn_status_validating:
            return "VALIDATING";
        case c_txn_status_committed:
            return "COMMITTED";
        case c_txn_status_aborted:
            return "ABORTED";
        default:
            ASSERT_UNREACHABLE("Unexpected txn metadata status flags!");
        }

        return nullptr;
    }

    inline void dump_metadata()
    {
        std::bitset<c_txn_metadata_bits> metadata_bits{m_word};

        std::cerr << "Transaction metadata bits: " << metadata_bits << std::endl;

        if (is_uninitialized())
        {
            std::cerr << "UNKNOWN" << std::endl;
            return;
        }

        if (is_sealed())
        {
            std::cerr << "INVALID" << std::endl;
            return;
        }

        std::cerr << "Type: " << (is_commit_ts_entry() ? "COMMIT_TIMESTAMP" : "BEGIN_TIMESTAMP") << std::endl;
        std::cerr << "Status: " << status_to_str() << std::endl;

        if (is_commit_ts_entry())
        {
            std::cerr << "Log fd for commit_ts metadata: " << get_log_fd() << std::endl;
        }
    }
};

} // namespace transactions
} // namespace db
} // namespace gaia
