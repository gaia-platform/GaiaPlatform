/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <random>

#include <gtest/gtest.h>

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/common/debug_assert.hpp"
#include "gaia_internal/common/timer.hpp"

#include <gaia_spdlog/fmt/fmt.h>

using namespace std;
using namespace gaia::common;

TEST(common, assert)
{
    try
    {
        ASSERT_INVARIANT(true, "Unexpected triggering of assert!");
        EXPECT_EQ(true, true);
    }
    catch (const std::exception& e)
    {
        EXPECT_EQ(true, false);
    }

    try
    {
        ASSERT_INVARIANT(false, "Expected triggering of assert.");
        EXPECT_EQ(true, false);
    }
    catch (const std::exception& e)
    {
        EXPECT_EQ(true, true);
        cerr << "Exception message: " << e.what() << '\n';
    }
}

TEST(common, debug_assert)
{
    try
    {
        DEBUG_ASSERT_INVARIANT(true, "Unexpected triggering of debug assert!");
        EXPECT_EQ(true, true);
    }
    catch (const std::exception& e)
    {
        EXPECT_EQ(true, false);
    }

    try
    {
        DEBUG_ASSERT_INVARIANT(false, "Expected (in debug builds only) triggering of debug assert.");
#ifdef DEBUG
        EXPECT_EQ(true, false);
#else
        EXPECT_EQ(true, true);
#endif
    }
    catch (const std::exception& e)
    {
#ifdef DEBUG
        EXPECT_EQ(true, true);
#else
        EXPECT_EQ(true, false);
#endif
        cerr << "Exception message: " << e.what() << '\n';
    }
}

using g_timer_t = gaia::common::timer_t;

TEST(common, retail_assert_perf)
{
    constexpr size_t c_condition = 10000;
    std::random_device dev;
    std::mt19937_64 rng(dev());
    std::uniform_int_distribution<int32_t> dist(0, c_condition);

    // Increase this value when using as a performance test.
    constexpr size_t c_num_samples = 10;
    int32_t numbers[c_num_samples];

    for (int32_t& number : numbers)
    {
        number = dist(rng);
    }

    auto start = g_timer_t::get_time_point();

    for (int number : numbers)
    {
        try
        {
            ASSERT_PRECONDITION(number >= 0, gaia_fmt::format("Concatenating a string {}", number).c_str());
        }
        catch (assertion_failure& e)
        {
        }
    }

    auto elapsed = g_timer_t::ns_to_ms(g_timer_t::get_duration(start));
    cout << "Elapsed: " << elapsed << endl;
}
