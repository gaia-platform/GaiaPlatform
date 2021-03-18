/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "txn_metadata.hpp"

#include <bitset>
#include <iostream>

#include "gaia_internal/common/mmap_helpers.hpp"

#include "db_helpers.hpp"

namespace gaia
{
namespace db
{

// Use a huge sparse array to store the txn metadata, where each entry is indexed by
// its begin or commit timestamp. We use 2^45 = 32TB of virtual address space
// for this array, but only the pages we actually use will ever be allocated.
// From https://www.kernel.org/doc/Documentation/vm/overcommit-accounting, it
// appears that MAP_NORESERVE is ignored in overcommit mode 2 (never
// overcommit), so we may need to check the `vm.overcommit_memory` sysctl and
// fail if it's set to 2. Alternatively, we could use the more tedious but
// reliable approach of using mmap(PROT_NONE) and calling
// mprotect(PROT_READ|PROT_WRITE) on any pages we actually need to use (this is
// analogous to VirtualAlloc(MEM_RESERVE) followed by VirtualAlloc(MEM_COMMIT)
// in Windows). 2^45 bytes of virtual address space holds 2^42 8-byte words, so
// we can allocate at most 2^42 timestamps over the lifetime of the process. If
// we allocate timestamps at a rate of 1M/s, we can run for about 2^22 seconds,
// or about a month and a half. If we allocate only 1K/s, we can run a thousand
// times longer: 2^32 seconds = ~128 years. If running out of timestamps is a
// concern, we can just convert the array to a circular buffer and store a
// wraparound counter separately.
//
// Each entry in the txn_metadata array is an 8-byte word, with the high bit
// indicating whether the index represents a begin or commit timestamp (they are
// allocated from the same counter). The next two bits indicate the current
// state of the txn state machine. The txn state machine's states and
// transitions are as follows: TXN_NONE -> TXN_ACTIVE -> (TXN_SUBMITTED ->
// (TXN_COMMITTED, TXN_ABORTED), TXN_TERMINATED). TXN_NONE is necessarily
// encoded as 0 because all entries of the array are zeroed when a page is
// allocated. We also introduce the pseudo-state TXN_DECIDED, encoded as the
// shared set bit in TXN_COMMITTED and TXN_ABORTED. Each active txn may assume
// the states TXN_NONE, TXN_ACTIVE, TXN_SUBMITTED, TXN_TERMINATED, while
// each submitted txn may assume the states TXN_SUBMITTED, TXN_COMMITTED,
// TXN_ABORTED, so we need 2 bits for each txn metadata entry to represent all possible
// states. Combined with the begin/commit bit, we need to reserve the 3 high
// bits of each 64-bit entry for txn state, leaving 61 bits for other purposes.
// The remaining bits are used as follows. In the TXN_NONE state, all bits are
// 0. In the TXN_ACTIVE state, the non-state bits are currently unused, but
// they could be used for active txn information like session thread ID, session
// socket, session userfaultfd, etc. In the TXN_SUBMITTED state, the remaining
// bits always hold a commit timestamp (which is guaranteed to be at most 42
// bits per above). The commit timestamp serves as a forwarding pointer to the
// submitted txn metadata for this txn. Note that the commit timestamp metadata for a
// txn may be set before its begin timestamp metadata has been updated, because
// multiple updates to the array cannot be made atomically without locks. (This
// is similar to insertion into a lock-free singly-linked list, where the new
// node is linked to its successor before its predecessor is linked to the new
// node.) Our algorithms are tolerant to this transient inconsistency. A commit
// timestamp metadata always holds the submitted txn's log fd in the low 16 bits
// following the 3 high bits used for state (we assume that all log fds fit into
// 16 bits), and the txn's begin timestamp in the low 42 bits.
//
// We can always find the watermark by just scanning the txn_metadata array until we
// find the first begin timestamp metadata not in state TXN_TERMINATED. However, we
// need to know where to start scanning from, because pages toward the beginning of
// the array may have already been deallocated (as part of advancing the
// watermark, once all txn logs preceding the watermark have been scanned for
// garbage and deallocated, all pages preceding the new watermark's position in
// the array can be freed using madvise(MADV_FREE)). We can store a global
// watermark variable to cache this information, which could always be stale but
// tells us where to start scanning to find the current watermark, because the
// watermark only moves forward. This global variable is also set as part of
// advancing the watermark on termination of the oldest active txn, which is
// delegated to that txn's session thread.
void txn_metadata_t::init_txn_metadata_map()
{
    // This should always be true on any 64-bit platform, but we assert because we
    // never want to fall back to a lock-based implementation of atomics.
    static_assert(
        std::atomic<txn_metadata_entry_t>::is_always_lock_free,
        "std::atomic<txn_metadata_entry_t> implementation was expected to be lock free!");

    // We reserve 2^45 bytes = 32TB of virtual address space. YOLO.
    constexpr size_t c_size_in_bytes = (1ULL << c_txn_ts_bits) * sizeof(*s_txn_metadata_map);

    // Create an anonymous private mapping with MAP_NORESERVE to indicate that
    // we don't care about reserving swap space.
    // REVIEW: If this causes problems on systems that disable overcommit, we
    // can just use PROT_NONE and mprotect(PROT_READ|PROT_WRITE) each page as we
    // need it.
    if (s_txn_metadata_map)
    {
        common::unmap_fd_data(s_txn_metadata_map, c_size_in_bytes);
    }

    common::map_fd_data(
        s_txn_metadata_map,
        c_size_in_bytes,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
        -1,
        0);
}

// This is designed for implementing "fences" that can guarantee no thread can
// ever claim a timestamp, by marking that timestamp permanently sealed.
// Sealing can only be performed on an "uninitialized" metadata entry, not on any valid
// metadata entry. When a session thread beginning or committing a txn finds that its
// begin_ts or commit_ts has been sealed upon initializing the metadata entry for
// that timestamp, it simply allocates another timestamp and retries. This is
// possible because we never publish a newly allocated timestamp until we know
// that its metadata entry has been successfully initialized.
bool txn_metadata_t::seal_uninitialized_ts(gaia_txn_id_t ts)
{
    // If the metadata is not uninitialized, we can't seal it.
    if (!is_uninitialized_ts(ts))
    {
        return false;
    }

    txn_metadata_entry_t expected_metadata_entry = c_value_uninitialized;
    txn_metadata_entry_t sealed_metadata_entry = c_value_sealed;

    bool has_sealed_metadata = s_txn_metadata_map[ts].compare_exchange_strong(
        expected_metadata_entry, sealed_metadata_entry);
    // We don't consider TXN_SUBMITTED or TXN_TERMINATED to be valid prior states, because only the
    // submitting thread can transition the txn to these states.
    if (!has_sealed_metadata)
    {
        // NB: expected_metadata is an inout argument holding the previous value on failure!
        common::retail_assert(
            expected_metadata_entry != c_value_uninitialized,
            "An uninitialized txn metadata cannot fail to be sealed!");
    }

    return has_sealed_metadata;
}

bool txn_metadata_t::invalidate_txn_log_fd(gaia_txn_id_t commit_ts)
{
    txn_metadata_t commit_ts_metadata(commit_ts);

    common::retail_assert(commit_ts_metadata.is_commit_ts(), c_message_not_a_commit_timestamp);
    common::retail_assert(commit_ts_metadata.is_decided(), "Cannot invalidate an undecided txn!");

    // The txn log fd is the 16 bits of the ts metadata before the final
    // 42 bits of the linked timestamp. We
    // don't zero these out because 0 is technically a valid fd (even though
    // it's normally reserved for stdin). Instead we follow the same convention
    // as elsewhere, using a reserved value of -1 to indicate an invalid fd.
    // (This means, of course, that we cannot use uint16_t::max() as a valid fd.)
    // We need a CAS because only one thread can be allowed to invalidate the fd
    // entry and hence to close the fd.
    // NB: we use compare_exchange_weak() for the global update because we need to
    // retry anyway on concurrent updates, so tolerating spurious failures
    // requires no additional logic.
    txn_metadata_t invalidated_commit_ts_metadata;
    do
    {
        // NB: commit_ts_metadata is an inout argument holding the previous value
        // on failure!
        if (commit_ts_metadata.get_txn_log_fd() == -1)
        {
            return false;
        }

        invalidated_commit_ts_metadata = commit_ts_metadata;
        invalidated_commit_ts_metadata.m_value |= c_txn_log_fd_mask;
    } while (!compare_exchange_weak(commit_ts_metadata, invalidated_commit_ts_metadata));

    return true;
}

// Sets the begin_ts metadata's status to TXN_SUBMITTED.
void txn_metadata_t::set_active_txn_submitted(gaia_txn_id_t begin_ts, gaia_txn_id_t commit_ts)
{
    // Only an active txn can be submitted.
    common::retail_assert(is_txn_active(begin_ts), "Not an active transaction!");
    common::retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

    // Transition the begin_ts metadata to the TXN_SUBMITTED state.
    constexpr uint64_t c_submitted_flags
        = c_txn_status_submitted << c_txn_status_flags_shift;

    // A submitted begin_ts metadata has the commit_ts in its low bits.
    txn_metadata_entry_t submitted_metadata_entry = (c_submitted_flags | commit_ts);

    // We don't need a CAS here because only the session thread can submit or terminate a txn,
    // and an active txn cannot be sealed.
    s_txn_metadata_map[begin_ts] = submitted_metadata_entry;
}

// Sets the begin_ts metadata's status to TXN_TERMINATED.
void txn_metadata_t::set_active_txn_terminated(gaia_txn_id_t begin_ts)
{
    // Only an active txn can be terminated.
    txn_metadata_t begin_ts_metadata(begin_ts);
    common::retail_assert(begin_ts_metadata.is_active(), "Not an active transaction!");

    constexpr uint64_t c_terminated_flags = c_txn_status_terminated << c_txn_status_flags_shift;

    txn_metadata_entry_t new_metadata_entry
        = c_terminated_flags | (begin_ts_metadata.m_value & ~c_txn_status_flags_mask);

    // We don't need a CAS here because only the session thread can submit or terminate a txn,
    // and an active txn cannot be sealed.
    s_txn_metadata_map[begin_ts] = new_metadata_entry;
}

void txn_metadata_t::update_txn_decision(gaia_txn_id_t commit_ts, bool is_committed)
{
    // The commit_ts metadata must be in state TXN_VALIDATING or TXN_DECIDED.
    // We allow the latter to enable idempotent concurrent validation.
    txn_metadata_t commit_ts_metadata(commit_ts);

    common::retail_assert(
        commit_ts_metadata.is_validating() || commit_ts_metadata.is_decided(),
        "commit_ts metadata must be in validating or decided state!");

    uint64_t decided_status_flags{is_committed ? c_txn_status_committed : c_txn_status_aborted};

    txn_metadata_t decided_commit_ts_metadata = commit_ts_metadata;

    // It's safe to just OR in the new flags since the preceding states don't set
    // any bits not present in the flags.
    decided_commit_ts_metadata.m_value |= (decided_status_flags << c_txn_status_flags_shift);

    bool has_set_metadata = compare_exchange_strong(commit_ts_metadata, decided_commit_ts_metadata);

    if (!has_set_metadata)
    {
        // The only metadata that could have changed between TXN_VALIDATING and TXN_DECIDED
        // is TXN_DURABLE (recursive validation executes concurrently with persistence).
        if (!commit_ts_metadata.is_decided())
        {
            common::retail_assert(
                commit_ts_metadata.is_durable(),
                "commit_ts metadata in validating state can only have its durable status changed if it is not decided!");

            decided_commit_ts_metadata = commit_ts_metadata;

            // It's safe to just OR in the new flags because the preceding states don't set
            // any bits not present in the flags.
            decided_commit_ts_metadata.m_value |= (decided_status_flags << c_txn_status_flags_shift);

            // Try to set the txn decision again. If it fails this time, it can
            // only be because the txn was validated concurrently.
            has_set_metadata = compare_exchange_strong(commit_ts_metadata, decided_commit_ts_metadata);
        }
    }

    if (!has_set_metadata)
    {
        // If the txn was already durable and the second CAS failed, then only
        // validation status could have changed. (We can't elide this check if
        // persistence is disabled, because we no longer have access to server
        // configuration.)
        common::retail_assert(
            commit_ts_metadata.is_decided(),
            "commit_ts metadata in validating state can only transition to a decided state if it is already durable!");

        // If another txn validated before us, they should have reached the same decision.
        common::retail_assert(
            commit_ts_metadata.is_committed() == is_committed,
            "Inconsistent txn decision detected!");
    }
}

void txn_metadata_t::set_txn_durable(gaia_txn_id_t commit_ts)
{
    txn_metadata_t commit_ts_metadata(commit_ts);

    txn_metadata_t durable_commit_ts_metadata;
    do
    {
        // NB: commit_ts_metadata is an inout argument holding the previous value
        // on failure!
        durable_commit_ts_metadata = commit_ts_metadata;

        durable_commit_ts_metadata.m_value |= (c_txn_persistence_complete << c_txn_persistence_flags_shift);

    } while (!compare_exchange_weak(commit_ts_metadata, durable_commit_ts_metadata));
}

bool txn_metadata_t::set_txn_gc_complete(gaia_txn_id_t commit_ts)
{
    txn_metadata_t commit_ts_metadata(commit_ts);
    txn_metadata_t expected_metadata = commit_ts_metadata;

    // Set GC status to TXN_GC_COMPLETE.
    commit_ts_metadata.m_value |= (c_txn_gc_complete << c_txn_gc_flags_shift);

    return compare_exchange_strong(expected_metadata, commit_ts_metadata);
}

// This method allocates a new begin_ts and initializes its metadata in the txn
// table.
gaia_txn_id_t txn_metadata_t::txn_begin()
{
    // The newly allocated begin timestamp for the new txn.
    gaia_txn_id_t begin_ts;

    // NB: expected_metadata is an inout argument holding the previous value on failure!
    txn_metadata_entry_t expected_metadata_entry;

    // The begin_ts metadata must have its status initialized to TXN_ACTIVE.
    // All other bits should be 0.
    const txn_metadata_entry_t c_begin_ts_metadata_entry = c_txn_status_active << c_txn_status_flags_shift;

    // Loop until we successfully install a newly allocated begin_ts in the txn
    // table. (We're possibly racing another beginning or committing txn that
    // could invalidate our begin_ts metadata before we install it.)
    //
    // NB: we use compare_exchange_weak() because we need to retry anyway on
    // concurrent invalidation, so tolerating spurious failures requires no
    // additional logic.
    do
    {
        // Allocate a new begin timestamp.
        begin_ts = db::allocate_txn_id();
        check_ts_size(begin_ts);

        // The txn metadata must be uninitialized (not sealed).
        expected_metadata_entry = c_value_uninitialized;
        // REVIEW: There should be an assert that expected_metadata_entry
        // can only have the "sealed" value if the CAS fails.
    } while (!s_txn_metadata_map[begin_ts].compare_exchange_weak(
        expected_metadata_entry, c_begin_ts_metadata_entry));

    return begin_ts;
}

gaia_txn_id_t txn_metadata_t::register_commit_ts(gaia_txn_id_t begin_ts, int log_fd)
{
    common::retail_assert(!is_uninitialized_ts(begin_ts), c_message_uninitialized_timestamp);

    // We construct the commit_ts metadata by concatenating required bits.
    uint64_t shifted_log_fd = static_cast<uint64_t>(log_fd) << c_txn_log_fd_shift;
    constexpr uint64_t c_shifted_status_flags
        = c_txn_status_validating << c_txn_status_flags_shift;
    txn_metadata_entry_t commit_ts_metadata_entry = c_shifted_status_flags | shifted_log_fd | begin_ts;

    // We're possibly racing another beginning or committing txn that wants to
    // seal our commit_ts metadata. We use compare_exchange_weak() because we
    // need to loop until success anyway. A spurious failure will just waste a
    // timestamp, and the uninitialized metadata will eventually be sealed.
    gaia_txn_id_t commit_ts;
    txn_metadata_entry_t expected_metadata_entry;
    do
    {
        // Allocate a new commit timestamp.
        commit_ts = db::allocate_txn_id();
        check_ts_size(commit_ts);

        // The commit_ts metadata must be uninitialized.
        expected_metadata_entry = c_value_uninitialized;
    } while (!s_txn_metadata_map[commit_ts].compare_exchange_weak(
        expected_metadata_entry, commit_ts_metadata_entry));

    return commit_ts;
}

void txn_metadata_t::dump_txn_metadata(gaia_txn_id_t ts)
{
    // NB: We generally cannot use the is_*_ts() functions because the entry could
    // change while we're reading it!
    txn_metadata_t txn_metadata(ts);
    std::bitset<c_txn_metadata_bits> metadata_bits(txn_metadata.m_value);

    std::cerr << "Transaction metadata for timestamp " << ts << ": " << metadata_bits << std::endl;

    if (txn_metadata.is_uninitialized())
    {
        std::cerr << "UNKNOWN" << std::endl;
        return;
    }

    if (txn_metadata.is_sealed())
    {
        std::cerr << "INVALID" << std::endl;
        return;
    }

    std::cerr << "Type: " << (txn_metadata.is_commit_ts() ? "COMMIT" : "ACTIVE") << std::endl;
    std::cerr << "Status: " << txn_metadata.status_to_str() << std::endl;

    if (txn_metadata.is_commit_ts())
    {
        gaia_txn_id_t begin_ts = get_begin_ts(ts);
        // We can't recurse here because we'd just bounce back and forth between a
        // txn's begin_ts and commit_ts.
        txn_metadata_t txn_metadata(begin_ts);
        std::bitset<c_txn_metadata_bits> metadata_bits(txn_metadata.m_value);
        std::cerr
            << "Timestamp metadata for commit_ts metadata's begin_ts " << begin_ts
            << ": " << metadata_bits << std::endl;
        std::cerr << "Log fd for commit_ts metadata: " << get_txn_log_fd(ts) << std::endl;
    }

    if (txn_metadata.is_begin_ts() && txn_metadata.is_submitted())
    {
        gaia_txn_id_t commit_ts = get_commit_ts(ts);
        if (commit_ts != c_invalid_gaia_txn_id)
        {
            // We can't recurse here because we'd just bounce back and forth between a
            // txn's begin_ts and commit_ts.
            txn_metadata_t txn_metadata(commit_ts);
            std::bitset<c_txn_metadata_bits> metadata_bits(txn_metadata.m_value);
            std::cerr
                << "Timestamp metadata for begin_ts metadata's commit_ts " << commit_ts
                << ": " << metadata_bits << std::endl;
        }
    }
}

txn_metadata_t::txn_metadata_t() noexcept
    : m_ts(c_invalid_gaia_txn_id), m_value(c_value_uninitialized)
{
}

txn_metadata_t::txn_metadata_t(gaia_txn_id_t ts)
    : m_ts(ts)
{
    common::retail_assert(m_ts != c_invalid_gaia_txn_id, "Invalid txn timestamp!");
    check_ts_size(m_ts);

    m_value = s_txn_metadata_map[m_ts];
}

const char* txn_metadata_t::status_to_str() const
{
    common::retail_assert(
        !is_uninitialized() && !is_sealed(),
        "Not a valid txn metadata!");

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
        common::retail_assert(false, "Unexpected txn_metadata_t status flags!");
    }

    return nullptr;
}

} // namespace db
} // namespace gaia
