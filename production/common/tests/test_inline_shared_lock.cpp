/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <atomic>

#include <gtest/gtest.h>

#include "gaia_internal/common/inline_shared_lock.hpp"

using namespace std;
using namespace gaia::common;

// TODO: add multithreaded tests!
TEST(common__inline_shared_lock__test, basic)
{
    inline_shared_lock lock;

    // Lock should be free after calling init_lock().
    EXPECT_TRUE(lock.is_free());

    // Acquire in exclusive mode.
    EXPECT_TRUE(lock.try_acquire_exclusive());
    EXPECT_TRUE(lock.is_exclusive());

    // Try to acquire exclusive lock while it is held.
    EXPECT_FALSE(lock.try_acquire_exclusive());
    EXPECT_TRUE(lock.is_exclusive());

    // Release exclusive lock.
    lock.release_exclusive();
    EXPECT_FALSE(lock.is_exclusive());
    EXPECT_TRUE(lock.is_free());

    // Release exclusive lock without it being acquired.
    EXPECT_THROW(lock.release_exclusive(), gaia::common::assertion_failure);

    // Acquire in shared mode.
    EXPECT_TRUE(lock.try_acquire_shared());
    EXPECT_TRUE(lock.is_shared());

    // Acquire again in shared mode.
    EXPECT_TRUE(lock.try_acquire_shared());
    EXPECT_TRUE(lock.is_shared());

    // Try to release exclusive lock in shared mode.
    EXPECT_THROW(lock.release_exclusive(), gaia::common::assertion_failure);
    EXPECT_TRUE(lock.is_shared());

    // Release shared lock.
    lock.release_shared();
    EXPECT_TRUE(lock.is_shared());

    // Release shared lock.
    lock.release_shared();
    EXPECT_FALSE(lock.is_shared());
    EXPECT_TRUE(lock.is_free());

    // Try to release shared lock after it is no longer held.
    EXPECT_THROW(lock.release_shared(), gaia::common::assertion_failure);
    EXPECT_FALSE(lock.is_shared());

    // Acquire in shared mode.
    EXPECT_TRUE(lock.try_acquire_shared());
    EXPECT_TRUE(lock.is_shared());

    // Acquire in intent exclusive mode.
    EXPECT_TRUE(lock.try_acquire_exclusive_intent());
    EXPECT_TRUE(lock.is_shared_with_exclusive_intent());

    // Try to acquire in shared mode while intent exclusive lock is held.
    EXPECT_FALSE(lock.try_acquire_shared());
    EXPECT_TRUE(lock.is_shared_with_exclusive_intent());

    // Release shared lock.
    lock.release_shared();
    EXPECT_TRUE(lock.is_free_with_exclusive_intent());

    // Upgrade from intent exclusive to exclusive mode.
    EXPECT_TRUE(lock.try_acquire_exclusive());
    EXPECT_TRUE(lock.is_exclusive());

    // Try to acquire lock in intent exclusive mode while exclusive lock is held.
    EXPECT_FALSE(lock.try_acquire_exclusive_intent());
    EXPECT_TRUE(lock.is_exclusive());

    // Release exclusive lock.
    lock.release_exclusive();

    // Lock should now be free.
    EXPECT_TRUE(lock.is_free());
}
