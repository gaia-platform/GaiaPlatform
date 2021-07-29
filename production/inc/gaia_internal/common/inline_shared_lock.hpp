/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <atomic>

#include "gaia_internal/common/retail_assert.hpp"

namespace gaia
{
namespace common
{

// This is a reader-writer lock embedded in a single 64-bit word, which uses
// only lock-free atomic operations, and is therefore suitable for use in shared
// memory. The semantics are purposefully highly constrained to the needs of the
// current client code (compaction). The lock can be acquired either in
// "exclusive" mode (1 "writer", 0 "readers"), "shared" mode (0 "writers", >0
// "readers"), or "exclusive intent" (0 "writers", any number of "readers").
//
// The word is divided into an "exclusive" bit, an "exclusive_intent" bit, and a
// "reader_count" stored in the remaining bits. A writer sets the "exclusive"
// bit in order to acquire the lock in exclusive mode, i.e., to exclude both
// readers and writers. Acquiring the lock in exclusive mode will fail if
// "reader_count" is nonzero. A reader increments "reader_count" in order to
// acquire the lock in shared mode and decrements "reader_count" in order to
// release their shared lock. Acquiring the lock in shared mode will fail if the
// "exclusive" or "exclusive_intent" bits are set. The "exclusive_intent" bit is
// set by a waiting writer in order to block further readers. It is expected
// that the waiting writer will wait for existing readers to exit, and then
// acquire the lock in exclusive mode (unless another thread has already done
// so). Acquiring the lock in "exclusive intent" mode will fail if the lock is
// already in either exclusive or "exclusive intent" mode. (It is allowed for
// one thread to set "exclusive intent" and another thread to subsequently
// acquire the lock in "exclusive" mode.)
//
// The following lock states are defined (note that the "exclusive" and
// "exclusive intent" states are mutually exclusive: a writer cannot block
// readers while waiting for another writer):
//
// FREE (all bits 0)
// FREE_WITH_EXCLUSIVE_INTENT (exclusive=0, exclusive_intent=1, reader_count=0)
// SHARED_WITH_EXCLUSIVE_INTENT (exclusive=0, exclusive_intent=1, reader_count > 0)
// EXCLUSIVE (exclusive=1, exclusive_intent=0, reader_count=0)
// SHARED (exclusive=0, exclusive_intent=0, reader_count > 0)
//
// Allowed transitions:
//
// FREE->EXCLUSIVE
// FREE->SHARED
// FREE->FREE_WITH_EXCLUSIVE_INTENT
// SHARED->SHARED
// SHARED->FREE
// SHARED->SHARED_WITH_EXCLUSIVE_INTENT
// SHARED_WITH_EXCLUSIVE_INTENT->SHARED_WITH_EXCLUSIVE_INTENT
// SHARED_WITH_EXCLUSIVE_INTENT->FREE_WITH_EXCLUSIVE_INTENT
// FREE_WITH_EXCLUSIVE_INTENT->EXCLUSIVE
// EXCLUSIVE->FREE
//
// State transition functions:
//
// try_acquire_exclusive->true if (FREE->EXCLUSIVE) or (FREE_WITH_EXCLUSIVE_INTENT->EXCLUSIVE) else false
// try_acquire_shared->true if (FREE->SHARED) or (SHARED->SHARED) else false
// try_acquire_exclusive_intent->true if (SHARED->SHARED_WITH_EXCLUSIVE_INTENT) or (FREE->FREE_WITH_EXCLUSIVE_INTENT) else false
// release_shared->void if (SHARED->SHARED) or (SHARED->FREE) or (SHARED_WITH_EXCLUSIVE_INTENT->SHARED_WITH_EXCLUSIVE_INTENT) or (SHARED_WITH_EXCLUSIVE_INTENT->FREE_WITH_EXCLUSIVE_INTENT) else assert
// release_exclusive->void if (EXCLUSIVE->FREE) else assert

class inline_shared_lock
{
public:
    bool is_free();
    bool is_exclusive();
    bool is_shared();
    bool is_free_with_exclusive_intent();
    bool is_shared_with_exclusive_intent();

    // NB: This can succeed when "exclusive intent" has been previously set by a
    // different thread than the caller!
    bool try_acquire_exclusive();
    bool try_acquire_exclusive_intent();
    bool try_acquire_shared();

    void release_exclusive();
    void release_shared();

    // NB: This cannot be used safely in program logic without synchronization,
    // and is intended only for logging/debugging.
    size_t get_reader_count();

private:
    static constexpr uint64_t c_free_lock{0};
    static constexpr uint64_t c_exclusive_bit_index{63};
    static constexpr uint64_t c_exclusive_mask{1ULL << c_exclusive_bit_index};
    static constexpr uint64_t c_exclusive_intent_bit_index{62};
    static constexpr uint64_t c_exclusive_intent_mask{1ULL << c_exclusive_intent_bit_index};
    static constexpr uint64_t c_reader_count_bits{62};
    static constexpr uint64_t c_reader_count_mask{(1ULL << c_reader_count_bits) - 1};
    static constexpr size_t c_reader_count_max{(1ULL << c_reader_count_bits) - 1};

private:
    std::atomic<uint64_t> m_lock_word{c_free_lock};

private:
    bool is_valid();
    void check_state();

    // These overloads are necessary to avoid races.
    static bool is_free(uint64_t lock_word);
    static bool is_exclusive(uint64_t lock_word);
    static bool is_shared(uint64_t lock_word);
    static bool is_free_with_exclusive_intent(uint64_t lock_word);
    static bool is_shared_with_exclusive_intent(uint64_t lock_word);
};

#include "inline_shared_lock.inc"

} // namespace common
} // namespace gaia
