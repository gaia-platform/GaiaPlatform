/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"
#include "event_guard.hpp"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;

TEST(event_guard_test, event_guard)
{
    uint32_t log_events = 0;
    event_type_t begin = event_type_t::transaction_begin;
    {
        // Mark begin event as running
        event_guard_t guard(log_events, begin);
        EXPECT_EQ(false, guard.is_blocked());
        {
            // Trying to run begin event again should
            // block it.
            event_guard_t guard(log_events, begin);
            EXPECT_EQ(true, guard.is_blocked());
            {
                event_type_t commit = event_type_t::transaction_commit;
                // Commit event should be allowed to run since it's a
                // different event type.
                event_guard_t guard(log_events, commit);
                EXPECT_EQ(false, guard.is_blocked());

                // Begin event and commit event are running
                EXPECT_EQ((uint32_t)begin | (uint32_t)commit, log_events);
            }
            // Begin event is still running but commit event is not.
            EXPECT_EQ((uint32_t)begin, log_events);
        }
        // Begin event is still running.
        EXPECT_EQ((uint32_t)begin, log_events);
    }
    // Finally begin event is done running.
    EXPECT_EQ(0, log_events);
}
