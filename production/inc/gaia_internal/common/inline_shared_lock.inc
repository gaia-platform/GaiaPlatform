////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

inline bool inline_shared_lock::is_free(uint64_t lock_word)
{
    return (lock_word == 0);
}

inline bool inline_shared_lock::is_exclusive(uint64_t lock_word)
{
    return (lock_word & c_exclusive_mask);
}

inline bool inline_shared_lock::is_shared(uint64_t lock_word)
{
    return !(lock_word & c_exclusive_intent_mask) && (lock_word & c_reader_count_mask);
}

inline bool inline_shared_lock::is_free_with_exclusive_intent(uint64_t lock_word)
{
    return (lock_word & c_exclusive_intent_mask) && !(lock_word & c_reader_count_mask);
}

inline bool inline_shared_lock::is_shared_with_exclusive_intent(uint64_t lock_word)
{
    return (lock_word & c_exclusive_intent_mask) && (lock_word & c_reader_count_mask);
}

inline bool inline_shared_lock::is_free()
{
    uint64_t lock_word = m_lock_word.load();
    return is_free(lock_word);
}

inline bool inline_shared_lock::is_exclusive()
{
    uint64_t lock_word = m_lock_word.load();
    return is_exclusive(lock_word);
}

inline bool inline_shared_lock::is_shared()
{
    uint64_t lock_word = m_lock_word.load();
    return is_shared(lock_word);
}

inline bool inline_shared_lock::is_free_with_exclusive_intent()
{
    uint64_t lock_word = m_lock_word.load();
    return is_free_with_exclusive_intent(lock_word);
}

inline bool inline_shared_lock::is_shared_with_exclusive_intent()
{
    uint64_t lock_word = m_lock_word.load();
    return is_shared_with_exclusive_intent(lock_word);
}

inline bool inline_shared_lock::is_valid()
{
    // All states are mutually exclusive, so exactly one state predicate must be true.

    // Form a bitvector of all state predicate values.
    uint64_t lock_word = m_lock_word.load();
    uint64_t predicate_values{
        static_cast<uint64_t>(is_free(lock_word))
        | (static_cast<uint64_t>(is_exclusive(lock_word)) << 1UL)
        | (static_cast<uint64_t>(is_shared(lock_word)) << 2UL)
        | (static_cast<uint64_t>(is_free_with_exclusive_intent(lock_word)) << 3UL)
        | (static_cast<uint64_t>(is_shared_with_exclusive_intent(lock_word)) << 4UL)};

    // Now check that exactly one predicate is true, by separately checking that
    // at least one predicate is true[1], and at most one predicate is true[2].
    // If the bitvector is nonzero, then [1] is true. If the bitvector is either
    // zero or a power of 2 (which we verify with the standard bit-twiddling
    // trick: `x & (x - 1) == 0`), then [2] is true.
    return (predicate_values != 0) && ((predicate_values & (predicate_values - 1)) == 0);
}

inline void inline_shared_lock::check_state()
{
    ASSERT_INVARIANT(is_valid(), "Invalid state for lock word!");
}

inline size_t inline_shared_lock::get_reader_count()
{
    size_t reader_count = 0;
    uint64_t lock_word = m_lock_word.load();
    if (is_shared(lock_word) || is_shared_with_exclusive_intent(lock_word))
    {
        reader_count = lock_word & c_reader_count_mask;
    }
    return reader_count;
}

inline bool inline_shared_lock::try_acquire_exclusive()
{
    uint64_t lock_word = m_lock_word.load();
    do
    {
        check_state();

        // Allowed state transitions: FREE->EXCLUSIVE,
        // FREE_WITH_EXCLUSIVE_INTENT->EXCLUSIVE.
        if (!is_free(lock_word) && !is_free_with_exclusive_intent(lock_word))
        {
            return false;
        }

    } while (!m_lock_word.compare_exchange_weak(lock_word, c_exclusive_mask));

    check_state();
    return true;
}

inline bool inline_shared_lock::try_acquire_shared()
{
    uint64_t lock_word = m_lock_word.load();
    uint64_t reader_count;
    do
    {
        check_state();

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
    } while (!m_lock_word.compare_exchange_weak(lock_word, reader_count));

    check_state();
    return true;
}

inline bool inline_shared_lock::try_acquire_exclusive_intent()
{
    uint64_t lock_word = m_lock_word.load();
    do
    {
        check_state();

        // Allowed state transitions: FREE->FREE_WITH_EXCLUSIVE_INTENT,
        // SHARED->SHARED_WITH_EXCLUSIVE_INTENT.
        if (!is_free(lock_word) && !is_shared(lock_word))
        {
            return false;
        }
    } while (!m_lock_word.compare_exchange_weak(lock_word, lock_word | c_exclusive_intent_mask));

    check_state();
    return true;
}

inline void inline_shared_lock::release_shared()
{
    // We could just use an atomic decrement here, but that would be incorrect
    // if other flags could be concurrently set which would invalidate our
    // decrement (like the exclusive_intent bit invalidates an increment).
    // An explicit CAS ensures this can't happen.
    uint64_t lock_word = m_lock_word.load();
    uint64_t reader_count;
    uint64_t new_lock_word;
    do
    {
        check_state();

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
    } while (!m_lock_word.compare_exchange_weak(lock_word, new_lock_word));

    check_state();
}

inline void inline_shared_lock::release_exclusive()
{
    check_state();
    // Allowed state transition: EXCLUSIVE->FREE.
    uint64_t expected_value = c_exclusive_mask;
    bool has_released_lock = m_lock_word.compare_exchange_strong(expected_value, c_free_lock);
    ASSERT_POSTCONDITION(has_released_lock, "Cannot release an exclusive lock that is not acquired!");
    check_state();
}

inline void inline_shared_lock::clear()
{
    m_lock_word.store(0);
}
