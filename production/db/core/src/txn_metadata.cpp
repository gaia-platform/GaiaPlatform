////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "txn_metadata.hpp"

#include <bitset>
#include <iostream>

#include "gaia/exceptions.hpp"

#include "gaia_internal/common/mmap_helpers.hpp"

#include "db_helpers.hpp"

namespace gaia
{
namespace db
{
namespace transactions
{

// Use a huge sparse array to store the txn metadata, where each entry is
// indexed by its begin or commit timestamp. We use 2^45 = 32TB of virtual
// address space for this array, but only the pages we actually use will ever be
// allocated. From
// https://www.kernel.org/doc/Documentation/vm/overcommit-accounting, it appears
// that MAP_NORESERVE is ignored in overcommit mode 2 (never overcommit), so we
// may need to check the `vm.overcommit_memory` sysctl and fail if it's set to
// 2. Alternatively, we could use the more tedious but reliable approach of
// using mmap(PROT_NONE) and calling mprotect(PROT_READ|PROT_WRITE) on any pages
// we actually need to use (this is analogous to VirtualAlloc(MEM_RESERVE)
// followed by VirtualAlloc(MEM_COMMIT) in Windows).
//
// 2^45 bytes of virtual address space holds 2^42 8-byte words, so we can
// allocate at most 2^42 timestamps over the lifetime of the process. If we
// allocate timestamps at a rate of 1M/s, we can run for about 2^22 seconds, or
// about a month and a half. If we allocate only 1K/s, we can run a thousand
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
// the states TXN_NONE, TXN_ACTIVE, TXN_SUBMITTED, TXN_TERMINATED, while each
// submitted txn may assume the states TXN_SUBMITTED, TXN_COMMITTED,
// TXN_ABORTED, so we need 2 bits for each txn metadata entry to represent all
// possible states. Combined with the begin/commit bit, we need to reserve the 3
// high bits of each 64-bit entry for txn state, leaving 61 bits for other
// purposes. The remaining bits are used as follows. In the TXN_NONE state, all
// bits are zero. In the TXN_ACTIVE state, the non-state bits are currently
// unused, but they could be used for active txn information like session thread
// ID, session socket, session userfaultfd, etc. In the TXN_SUBMITTED state, the
// remaining bits always hold a commit timestamp (which is guaranteed to be at
// most 42 bits per above). The commit timestamp serves as a forwarding pointer
// to the submitted txn metadata for this txn. Note that the commit timestamp
// metadata for a txn may be set before its begin timestamp metadata has been
// updated, because multiple updates to the array cannot be made atomically
// without locks. (This is similar to insertion into a lock-free singly-linked
// list, where the new node is linked to its successor before its predecessor is
// linked to the new node.) Our algorithms are tolerant to this transient
// inconsistency. A commit timestamp metadata entry always holds the submitted
// txn's log offset in the low 16 bits following the 3 high bits used for state
// (we assume that all log offsets fit into 16 bits), and the txn's begin
// timestamp in the low 42 bits.
//
// We can always find the watermark by just scanning the txn_metadata array
// until we find the first begin timestamp metadata not in state TXN_TERMINATED.
// However, we need to know where to start scanning from, because pages toward
// the beginning of the array may have already been deallocated (as part of
// advancing the watermark, once all txn logs preceding the watermark have been
// scanned for garbage and deallocated, all pages preceding the new watermark's
// position in the array can be freed using madvise(MADV_FREE)). We can store a
// global watermark variable to cache this information, which could always be
// stale but tells us where to start scanning to find the current watermark,
// because the watermark only moves forward. This global variable is also set as
// part of advancing the watermark on termination of the oldest active txn,
// which is delegated to that txn's session thread.
void txn_metadata_t::init_txn_metadata_map()
{
    // We reserve 2^45 bytes = 32TB of virtual address space. YOLO.
    constexpr size_t c_size_in_bytes{
        txn_metadata_entry_t::get_max_ts_count() * sizeof(*s_txn_metadata_map)};

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

bool txn_metadata_t::is_txn_metadata_map_initialized()
{
    return (s_txn_metadata_map != nullptr);
}

char* txn_metadata_t::get_txn_metadata_map_base_address()
{
    ASSERT_PRECONDITION(is_txn_metadata_map_initialized(), "Txn metadata map is uninitialized!");
    return reinterpret_cast<char*>(s_txn_metadata_map);
}

// This method allocates a new begin_ts and initializes its metadata in the txn
// table.
gaia_txn_id_t txn_metadata_t::register_begin_ts()
{
    // The newly allocated begin timestamp for the new txn.
    gaia_txn_id_t begin_ts;

    // Loop until we successfully install a newly allocated begin_ts in the txn
    // table. (We're possibly racing another beginning or committing txn that
    // could seal our begin_ts metadata entry before we install it.)
    // Technically, there is no bound on the number of iterations until success,
    // so this is not wait-free, but in practice conflicts should be very rare.
    while (true)
    {
        // Allocate a new begin timestamp.
        begin_ts = db::allocate_txn_id();
        txn_metadata_entry_t::check_ts_size(begin_ts);
        txn_metadata_t begin_ts_metadata{begin_ts};

        // The txn metadata must be uninitialized (not sealed).
        txn_metadata_entry_t expected_value{
            txn_metadata_entry_t::uninitialized_value()};
        txn_metadata_entry_t desired_value{
            txn_metadata_entry_t::new_begin_ts_entry()};
        txn_metadata_entry_t actual_value{
            begin_ts_metadata.compare_exchange(expected_value, desired_value)};

        if (actual_value == expected_value)
        {
            break;
        }

        // The CAS can only fail if it returns the "sealed" value.
        ASSERT_INVARIANT(
            actual_value == txn_metadata_entry_t::sealed_value(),
            "A newly allocated timestamp cannot be concurrently initialized to any value except the sealed value!");
    }

    return begin_ts;
}

gaia_txn_id_t txn_metadata_t::register_commit_ts(gaia_txn_id_t begin_ts, db::log_offset_t log_offset)
{
    ASSERT_PRECONDITION(!is_uninitialized_ts(begin_ts), c_message_uninitialized_timestamp);

    // The newly allocated commit timestamp for the submitted txn.
    gaia_txn_id_t commit_ts;

    // Loop until we successfully install a newly allocated commit_ts in the txn
    // table. (We're possibly racing another beginning or committing txn that
    // could seal our commit_ts metadata entry before we install it.)
    // Technically, there is no bound on the number of iterations until success,
    // so this is not wait-free, but in practice conflicts should be very rare.
    while (true)
    {
        // Allocate a new commit timestamp.
        commit_ts = db::allocate_txn_id();
        txn_metadata_entry_t::check_ts_size(commit_ts);
        txn_metadata_t commit_ts_metadata{commit_ts};

        // The txn metadata must be uninitialized (not sealed).
        txn_metadata_entry_t expected_value{
            txn_metadata_entry_t::uninitialized_value()};
        txn_metadata_entry_t desired_value{
            txn_metadata_entry_t::new_commit_ts_entry(begin_ts, log_offset)};
        txn_metadata_entry_t actual_value{
            commit_ts_metadata.compare_exchange(expected_value, desired_value)};

        if (actual_value == expected_value)
        {
            break;
        }

        // The CAS can only fail if it returns the "sealed" value.
        ASSERT_INVARIANT(
            actual_value == txn_metadata_entry_t::sealed_value(),
            "A newly allocated timestamp cannot be concurrently initialized to any value except the sealed value!");
    }

    return commit_ts;
}

void txn_metadata_t::dump_txn_metadata_at_ts(gaia_txn_id_t ts)
{
    txn_metadata_t ts_metadata(ts);
    std::cerr << ts_metadata.get_entry().dump_metadata();

    if (ts_metadata.is_commit_ts())
    {
        txn_metadata_t begin_ts_metadata = ts_metadata.get_begin_ts_metadata();
        std::cerr
            << "Metadata for commit_ts `" << ts << "` metadata's begin_ts `"
            << begin_ts_metadata.get_timestamp() << "`: " << std::endl
            << begin_ts_metadata.get_entry().dump_metadata();
    }

    if (ts_metadata.is_begin_ts())
    {
        // A begin_ts may not have a linked commit_ts, either because it is not
        // submitted or because it has not yet been updated with its commit_ts.
        auto opt_commit_ts_metadata = ts_metadata.get_commit_ts_metadata();
        if (opt_commit_ts_metadata)
        {
            txn_metadata_t commit_ts_metadata = *opt_commit_ts_metadata;
            std::cerr
                << "Metadata for begin_ts `" << ts << "` metadata's commit_ts `"
                << commit_ts_metadata.get_timestamp() << "`: " << std::endl
                << commit_ts_metadata.get_entry().dump_metadata();
        }
    }
}

} // namespace transactions
} // namespace db
} // namespace gaia
