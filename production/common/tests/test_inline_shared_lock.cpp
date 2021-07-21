/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <atomic>

#include "gtest/gtest.h"

#include "gaia_internal/common/inline_shared_lock.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::common::inline_shared_lock;

// TODO: add multithreaded tests!
TEST(common, inline_shared_lock)
{
    atomic<uint64_t> lock;
    init_lock(lock);

    // Lock should be free after calling init_lock().
    EXPECT_TRUE(is_free(lock));
    // Acquire in exclusive mode.
    EXPECT_TRUE(try_acquire_exclusive(lock));
    EXPECT_TRUE(is_exclusive(lock));
    // Try to acquire exclusive lock while it is held.
    EXPECT_FALSE(try_acquire_exclusive(lock));
    EXPECT_TRUE(is_exclusive(lock));
    // Release exclusive lock.
    release_exclusive(lock);
    EXPECT_FALSE(is_exclusive(lock));
    // Release exclusive lock without it being acquired.
    EXPECT_THROW(release_exclusive(lock), gaia::common::retail_assertion_failure);
    // Acquire in shared mode.
    EXPECT_TRUE(try_acquire_shared(lock));
    EXPECT_TRUE(is_shared(lock));
    // Acquire again in shared mode.
    EXPECT_TRUE(try_acquire_shared(lock));
    EXPECT_TRUE(is_shared(lock));
    // Try to release exclusive lock in shared mode.
    EXPECT_THROW(release_exclusive(lock), gaia::common::retail_assertion_failure);
    EXPECT_TRUE(is_shared(lock));
    // Release shared lock.
    release_shared(lock);
    EXPECT_TRUE(is_shared(lock));
    // Release shared lock.
    release_shared(lock);
    EXPECT_FALSE(is_shared(lock));
    // Try to release shared lock after it is no longer held.
    EXPECT_THROW(release_shared(lock), gaia::common::retail_assertion_failure);
    EXPECT_FALSE(is_shared(lock));
    // Acquire in shared mode.
    EXPECT_TRUE(try_acquire_shared(lock));
    EXPECT_TRUE(is_shared(lock));
    // Acquire in intent exclusive mode.
    EXPECT_TRUE(try_acquire_intent_exclusive(lock));
    EXPECT_TRUE(is_intent_exclusive_shared(lock));
    // Try to acquire in shared mode while intent exclusive lock is held.
    EXPECT_FALSE(try_acquire_shared(lock));
    EXPECT_TRUE(is_intent_exclusive_shared(lock));
    // Release shared lock.
    release_shared(lock);
    EXPECT_TRUE(is_intent_exclusive_free(lock));
    // Upgrade from intent exclusive to exclusive mode.
    EXPECT_TRUE(try_acquire_exclusive(lock));
    EXPECT_TRUE(is_exclusive(lock));
    // Try to acquire lock in intent exclusive mode while exclusive lock is held.
    EXPECT_FALSE(try_acquire_intent_exclusive(lock));
    EXPECT_TRUE(is_exclusive(lock));
    // Release exclusive lock.
    release_exclusive(lock);
    // Lock should now be free.
    EXPECT_TRUE(is_free(lock));
}
