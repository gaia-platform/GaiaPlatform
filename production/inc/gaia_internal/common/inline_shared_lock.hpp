/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <atomic>

#include "gaia_internal/common/retail_assert.hpp"

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

namespace gaia
{
namespace common
{
namespace inline_shared_lock
{

constexpr uint64_t c_free_lock{0};
constexpr uint64_t c_exclusive_bit_index{63};
constexpr uint64_t c_exclusive_mask{1ULL << c_exclusive_bit_index};
constexpr uint64_t c_exclusive_intent_bit_index{62};
constexpr uint64_t c_exclusive_intent_mask{1ULL << c_exclusive_intent_bit_index};
constexpr uint64_t c_reader_count_bits{62};
constexpr uint64_t c_reader_count_mask{(1ULL << c_reader_count_bits) - 1};
constexpr size_t c_reader_count_max{(1ULL << c_reader_count_bits) - 1};

bool is_free(uint64_t lock_word)
{
    return (lock_word == 0);
}

bool is_exclusive(uint64_t lock_word)
{
    return (lock_word & c_exclusive_mask);
}

bool is_shared(uint64_t lock_word)
{
    return !(lock_word & c_exclusive_intent_mask) && (lock_word & c_reader_count_mask);
}

bool is_free_with_exclusive_intent(uint64_t lock_word)
{
    return (lock_word & c_exclusive_intent_mask) && !(lock_word & c_reader_count_mask);
}

bool is_shared_with_exclusive_intent(uint64_t lock_word)
{
    return (lock_word & c_exclusive_intent_mask) && (lock_word & c_reader_count_mask);
}

bool is_valid(uint64_t lock_word)
{
    // All states are mutually exclusive, so exactly one state predicate must be true.

    // Form a bitvector of all state predicate values.
    uint64_t predicate_values{
        static_cast<uint64_t>(is_free(lock_word))
        | (static_cast<uint64_t>(is_exclusive(lock_word)) << 1ULL)
        | (static_cast<uint64_t>(is_shared(lock_word)) << 2ULL)
        | (static_cast<uint64_t>(is_free_with_exclusive_intent(lock_word)) << 3ULL)
        | (static_cast<uint64_t>(is_shared_with_exclusive_intent(lock_word)) << 4ULL)};

    // Now check that exactly one predicate is true, by separately checking that
    // at least one predicate is true[1], and at most one predicate is true[2].
    // If the bitvector is nonzero, then [1] is true. If the bitvector is either
    // zero or a power of 2 (which we verify with the standard bit-twiddling
    // trick: `x & (x - 1) == 0`), then [2] is true.
    return (predicate_values != 0) && ((predicate_values & (predicate_values - 1)) == 0);
}

void check_state(uint64_t lock_word)
{
    ASSERT_INVARIANT(is_valid(lock_word), "Invalid state for lock word!");
}

// NB: This cannot be used safely in program logic without synchronization, and
// is intended only for logging/debugging.
size_t get_reader_count(std::atomic<uint64_t>& lock)
{
    size_t reader_count = 0;
    uint64_t lock_word = lock.load();
    if (is_shared(lock_word) || is_shared_with_exclusive_intent(lock_word))
    {
        reader_count = lock_word & c_reader_count_mask;
    }
    return reader_count;
}

void init_lock(std::atomic<uint64_t>& lock)
{
    lock.store(c_free_lock);
}

// NB: This can succeed when "exclusive intent" has been previously set by a
// different thread than the caller!
bool try_acquire_exclusive(std::atomic<uint64_t>& lock)
{
    uint64_t lock_word = lock.load();
    do
    {
        check_state(lock_word);

        // Allowed state transitions: FREE->EXCLUSIVE,
        // FREE_WITH_EXCLUSIVE_INTENT->EXCLUSIVE.
        if (!is_free(lock_word) && !is_free_with_exclusive_intent(lock_word))
        {
            return false;
        }

    } while (!lock.compare_exchange_weak(lock_word, c_exclusive_mask));

    check_state(lock_word);
    return true;
}

bool try_acquire_shared(std::atomic<uint64_t>& lock)
{
    uint64_t lock_word = lock.load();
    uint64_t reader_count;
    do
    {
        check_state(lock_word);

        // Allowed state transitions: FREE->SHARED, SHARED->SHARED.
        if (!is_free(lock_word) && !is_shared(lock_word))
        {
            return false;
        }

        reader_count = lock_word & c_reader_count_mask;
        ASSERT_PRECONDITION(reader_count < c_reader_count_max, "Reader count must fit into 62 bits!");

        // This will never be reached unless asserts are disabled.
        if (reader_count >= c_reader_count_max)
        {
            return false;
        }

        reader_count += 1;
    } while (!lock.compare_exchange_weak(lock_word, reader_count));

    check_state(lock_word);
    return true;
}

bool try_acquire_exclusive_intent(std::atomic<uint64_t>& lock)
{
    uint64_t lock_word = lock.load();
    do
    {
        check_state(lock_word);

        // Allowed state transitions: FREE->FREE_WITH_EXCLUSIVE_INTENT,
        // SHARED->SHARED_WITH_EXCLUSIVE_INTENT.
        if (!is_free(lock_word) && !is_shared(lock_word))
        {
            return false;
        }
    } while (!lock.compare_exchange_weak(lock_word, lock_word | c_exclusive_intent_mask));

    check_state(lock_word);
    return true;
}

void release_shared(std::atomic<uint64_t>& lock)
{
    // We could just use an atomic decrement here, but that would be incorrect
    // if other flags could be concurrently set which would invalidate our
    // decrement (like the exclusive_intent bit invalidates an increment).
    // An explicit CAS ensures this can't happen.
    uint64_t lock_word = lock.load();
    uint64_t reader_count;
    uint64_t new_lock_word;
    do
    {
        check_state(lock_word);

        // Allowed state transitions: SHARED->SHARED, SHARED->FREE,
        // SHARED_WITH_EXCLUSIVE_INTENT->SHARED_WITH_EXCLUSIVE_INTENT,
        // SHARED_WITH_EXCLUSIVE_INTENT->FREE_WITH_EXCLUSIVE_INTENT.
        ASSERT_PRECONDITION(is_shared(lock_word) || is_shared_with_exclusive_intent(lock_word), "Cannot release a shared lock that is not acquired!");

        reader_count = lock_word & c_reader_count_mask;
        ASSERT_PRECONDITION(reader_count != 0, "Reader count must be nonzero!");
        reader_count -= 1;

        // Clear the reader count.
        new_lock_word = lock_word & ~c_reader_count_mask;
        // Fill in the decremented reader count.
        new_lock_word |= reader_count;
    } while (!lock.compare_exchange_weak(lock_word, new_lock_word));

    check_state(lock_word);
}

void release_exclusive(std::atomic<uint64_t>& lock)
{
    check_state(lock.load());
    // Allowed state transition: EXCLUSIVE->FREE.
    uint64_t expected_value = c_exclusive_mask;
    bool has_released_lock = lock.compare_exchange_strong(expected_value, c_free_lock);
    ASSERT_POSTCONDITION(has_released_lock, "Cannot release an exclusive lock that is not acquired!");
    check_state(lock.load());
}

} // namespace inline_shared_lock
} // namespace common
} // namespace gaia
