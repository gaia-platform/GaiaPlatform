/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <atomic>
#include <limits>
#include <optional>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "txn_metadata.inc"
#include "txn_metadata_entry.hpp"

namespace gaia
{
namespace db
{
namespace transactions
{

// This identity class represents mutable txn metadata at an immutable
// (begin or commit) timestamp.
// "A value class has a public destructor, copy constructor, and assignment with value semantics.""
class txn_metadata_t
{
private:
    const gaia_txn_id_t m_ts;

public:
    static void init_txn_metadata_map();

    inline explicit txn_metadata_t(gaia_txn_id_t ts)
        : m_ts(ts)
    {
        ASSERT_PRECONDITION(m_ts != c_invalid_gaia_txn_id, "Invalid txn timestamp!");
        txn_metadata_entry_t::check_ts_size(m_ts);
    }

    txn_metadata_t(const txn_metadata_t&) = default;

    // The copy assignment operator is implicitly deleted because this class has
    // no non-static, non-const members, but we make it explicit.
    txn_metadata_t& operator=(const txn_metadata_t&) = delete;

    friend inline bool operator==(txn_metadata_t a, txn_metadata_t b)
    {
        return (a.m_ts == b.m_ts);
    }

    friend inline bool operator!=(txn_metadata_t a, txn_metadata_t b)
    {
        return (a.m_ts != b.m_ts);
    }

    inline gaia_txn_id_t get_timestamp()
    {
        return m_ts;
    }

    inline bool is_uninitialized()
    {
        return get_entry().is_uninitialized();
    }

    inline bool is_sealed()
    {
        return get_entry().is_sealed();
    }

    inline bool is_begin_ts()
    {
        return get_entry().is_begin_ts_entry();
    }

    inline bool is_commit_ts()
    {
        return get_entry().is_commit_ts_entry();
    }

    inline bool is_submitted()
    {
        return get_entry().is_submitted();
    }

    inline bool is_validating()
    {
        return get_entry().is_validating();
    }

    inline bool is_decided()
    {
        return get_entry().is_decided();
    }

    inline bool is_committed()
    {
        return get_entry().is_committed();
    }

    inline bool is_aborted()
    {
        return get_entry().is_aborted();
    }

    inline bool is_gc_complete()
    {
        return get_entry().is_gc_complete();
    }

    inline bool is_durable()
    {
        return get_entry().is_durable();
    }

    inline bool is_active()
    {
        return get_entry().is_active();
    }

    inline bool is_terminated()
    {
        return get_entry().is_terminated();
    }

    static inline bool is_uninitialized_ts(gaia_txn_id_t ts)
    {
        return txn_metadata_t(ts).is_uninitialized();
    }

    static inline bool is_sealed_ts(gaia_txn_id_t ts)
    {
        return txn_metadata_t(ts).is_sealed();
    }

    static inline bool is_begin_ts(gaia_txn_id_t ts)
    {
        return txn_metadata_t(ts).is_begin_ts();
    }

    static inline bool is_commit_ts(gaia_txn_id_t ts)
    {
        return txn_metadata_t(ts).is_commit_ts();
    }

    static inline bool is_txn_submitted(gaia_txn_id_t begin_ts)
    {
        return txn_metadata_t(begin_ts).is_submitted();
    }

    static inline bool is_txn_validating(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).is_validating();
    }

    static inline bool is_txn_decided(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).is_decided();
    }

    static inline bool is_txn_committed(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).is_committed();
    }

    static inline bool is_txn_aborted(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).is_aborted();
    }

    static inline bool is_txn_gc_complete(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).is_gc_complete();
    }

    static inline bool is_txn_durable(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).is_durable();
    }

    static inline bool is_txn_active(gaia_txn_id_t begin_ts)
    {
        return txn_metadata_t(begin_ts).is_active();
    }

    static inline bool is_txn_terminated(gaia_txn_id_t begin_ts)
    {
        return txn_metadata_t(begin_ts).is_terminated();
    }

    inline txn_metadata_t get_begin_ts_metadata()
    {
        ASSERT_PRECONDITION(is_commit_ts(), "Not a commit timestamp!");
        return txn_metadata_t(get_entry().get_timestamp());
    }

    // This returns an optional value because a begin_ts entry will not contain
    // a commit_ts unless it has been submitted, and even then it may not because
    // a begin_ts entry is not updated atomically with its commit_ts entry.
    inline std::optional<txn_metadata_t> get_commit_ts_metadata()
    {
        ASSERT_PRECONDITION(is_begin_ts(), "Not a begin timestamp!");
        gaia_txn_id_t commit_ts = get_entry().get_timestamp();
        if (commit_ts != c_invalid_gaia_txn_id)
        {
            return txn_metadata_t(commit_ts);
        }
        return std::nullopt;
    }

    static inline gaia_txn_id_t get_begin_ts(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).get_begin_ts_metadata().get_timestamp();
    }

    static inline gaia_txn_id_t get_commit_ts(gaia_txn_id_t begin_ts)
    {
        auto commit_ts_metadata = txn_metadata_t(begin_ts).get_commit_ts_metadata();
        return commit_ts_metadata ? commit_ts_metadata->get_timestamp() : c_invalid_gaia_txn_id;
    }

    inline int get_txn_log_fd()
    {
        return get_entry().get_log_fd();
    }

    inline bool invalidate_txn_log_fd()
    {
        // We need a CAS because only one thread can be allowed to invalidate the fd
        // entry and hence to close the fd.
        // NB: we use compare_exchange_weak() for the global update because we need to
        // retry anyway on concurrent updates, so tolerating spurious failures
        // requires no additional logic.
        while (true)
        {
            if (get_txn_log_fd() == -1)
            {
                return false;
            }

            txn_metadata_entry_t expected_value{get_entry()};
            txn_metadata_entry_t desired_value{expected_value.invalidate_log_fd()};
            txn_metadata_entry_t actual_value{compare_exchange_weak(
                expected_value, desired_value)};

            if (actual_value == expected_value)
            {
                break;
            }
        }

        return true;
    }

    inline void set_active_txn_submitted(gaia_txn_id_t commit_ts)
    {
        // Only an active txn can be submitted.
        ASSERT_PRECONDITION(is_active(), "Not an active transaction!");
        ASSERT_PRECONDITION(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

        // We don't need a CAS here because only the session thread can submit or terminate a txn,
        // and an active txn cannot be sealed.
        set_entry(get_entry().set_submitted(commit_ts));
    }

    inline void set_active_txn_terminated()
    {
        // Only an active txn can be terminated.
        ASSERT_PRECONDITION(is_active(), "Not an active transaction!");

        // We don't need a CAS here because only the session thread can submit or terminate a txn,
        // and an active txn cannot be sealed.
        set_entry(get_entry().set_terminated());
    }

    inline void update_txn_decision(bool has_committed)
    {
        txn_metadata_entry_t expected_value{get_entry()};
        txn_metadata_entry_t desired_value{expected_value.set_decision(has_committed)};
        if (expected_value != compare_exchange_strong(expected_value, desired_value))
        {
            // The only state transition allowed from TXN_VALIDATING is to TXN_DECIDED.
            ASSERT_POSTCONDITION(
                is_decided(),
                "commit_ts metadata in validating state can only transition to a decided state!");

            // If another txn validated before us, they should have reached the same decision.
            ASSERT_POSTCONDITION(
                is_committed() == has_committed,
                "Inconsistent txn decision detected!");
        }
    }

    inline void set_txn_durable()
    {
        while (true)
        {
            txn_metadata_entry_t expected_value{get_entry()};
            txn_metadata_entry_t desired_value{expected_value.set_durable()};
            txn_metadata_entry_t actual_value{compare_exchange_weak(expected_value, desired_value)};

            if (actual_value == expected_value)
            {
                break;
            }
        }
    }

    inline bool set_txn_gc_complete()
    {
        txn_metadata_entry_t expected_value{get_entry()};
        txn_metadata_entry_t desired_value{expected_value.set_gc_complete()};
        txn_metadata_entry_t actual_value{compare_exchange_strong(expected_value, desired_value)};
        return (actual_value == expected_value);
    }

    // This is designed for implementing "fences" that can guarantee no thread can
    // ever claim a timestamp, by marking that timestamp permanently sealed. Sealing
    // can only be performed on an "uninitialized" metadata entry, not on any valid
    // metadata entry. When a session thread beginning or committing a txn finds
    // that its begin_ts or commit_ts has been sealed upon initializing the metadata
    // entry for that timestamp, it simply allocates another timestamp and retries.
    // This is possible because we never publish a newly allocated timestamp until
    // we know that its metadata entry has been successfully initialized.
    inline bool seal_if_uninitialized()
    {
        // If the metadata is not uninitialized, we can't seal it.
        if (!is_uninitialized())
        {
            return false;
        }

        txn_metadata_entry_t expected_value{txn_metadata_entry_t::uninitialized_value()};
        txn_metadata_entry_t desired_value{txn_metadata_entry_t::sealed_value()};
        txn_metadata_entry_t actual_value{compare_exchange_strong(expected_value, desired_value)};

        if (actual_value != expected_value)
        {
            // We don't consider TXN_SUBMITTED or TXN_TERMINATED to be valid prior
            // states, because only the submitting thread can transition the txn to
            // these states.
            ASSERT_INVARIANT(
                actual_value != txn_metadata_entry_t::uninitialized_value(),
                "An uninitialized txn metadata entry cannot fail to be sealed!");
            return false;
        }

        return true;
    }

    static inline int get_txn_log_fd(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).get_txn_log_fd();
    }

    static inline bool seal_uninitialized_ts(gaia_txn_id_t ts)
    {
        return txn_metadata_t(ts).seal_if_uninitialized();
    }

    static bool invalidate_txn_log_fd(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).invalidate_txn_log_fd();
    }

    static void set_active_txn_submitted(gaia_txn_id_t begin_ts, gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(begin_ts).set_active_txn_submitted(commit_ts);
    }

    static void set_active_txn_terminated(gaia_txn_id_t begin_ts)
    {
        return txn_metadata_t(begin_ts).set_active_txn_terminated();
    }

    static void update_txn_decision(gaia_txn_id_t commit_ts, bool is_committed)
    {
        return txn_metadata_t(commit_ts).update_txn_decision(is_committed);
    }

    static void set_txn_durable(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).set_txn_durable();
    }

    static bool set_txn_gc_complete(gaia_txn_id_t commit_ts)
    {
        return txn_metadata_t(commit_ts).set_txn_gc_complete();
    }

    static gaia_txn_id_t txn_begin();
    static gaia_txn_id_t register_commit_ts(gaia_txn_id_t begin_ts, int log_fd);

    static void dump_txn_metadata_at_ts(gaia_txn_id_t ts);

private:
    inline txn_metadata_entry_t get_entry() const
    {
        return txn_metadata_entry_t{s_txn_metadata_map[m_ts].load()};
    }

    inline void set_entry(txn_metadata_entry_t entry)
    {
        s_txn_metadata_map[m_ts].store(entry.get_word());
    }

    // These wrappers over std::atomic::compare_exchange_*() return the actual value of this txn_metadata_t instance when the method was called.
    // If the returned value is not equal to the expected value, then the CAS must have failed.

    inline txn_metadata_entry_t compare_exchange_strong(txn_metadata_entry_t expected_value, txn_metadata_entry_t desired_value)
    {
        auto expected_word = expected_value.get_word();
        auto desired_word = desired_value.get_word();
        s_txn_metadata_map[m_ts].compare_exchange_strong(expected_word, desired_word);
        // expected_word is passed by reference, and on exit holds the initial word at the address.
        txn_metadata_entry_t actual_value{expected_word};
        return actual_value;
    }

    inline txn_metadata_entry_t compare_exchange_weak(txn_metadata_entry_t expected_value, txn_metadata_entry_t desired_value)
    {
        auto expected_word = expected_value.get_word();
        auto desired_word = desired_value.get_word();
        s_txn_metadata_map[m_ts].compare_exchange_weak(expected_word, desired_word);
        // expected_word is passed by reference, and on exit holds the initial word at the address.
        txn_metadata_entry_t actual_value{expected_word};
        return actual_value;
    }

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
    // REVIEW: Because we reserve 2^45 bytes of virtual address space and each
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
    static inline std::atomic<uint64_t>* s_txn_metadata_map = nullptr;
};

} // namespace transactions
} // namespace db
} // namespace gaia
