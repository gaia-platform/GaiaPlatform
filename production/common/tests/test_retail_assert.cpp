/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <random>

#include <gtest/gtest.h>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/timer.hpp"

#include <gaia_spdlog/fmt/fmt.h>

using namespace std;
using namespace gaia::common;

TEST(common, retail_assert)
{
    try
    {
        ASSERT_INVARIANT(true, "Unexpected triggering of retail assert!");
        EXPECT_EQ(true, true);
    }
    catch (const std::exception& e)
    {
        EXPECT_EQ(true, false);
    }

    try
    {
        ASSERT_INVARIANT(false, "Expected triggering of retail assert.");
        EXPECT_EQ(true, false);
    }
    catch (const std::exception& e)
    {
        EXPECT_EQ(true, true);
        cerr << "Exception message: " << e.what() << '\n';
    }
}

using g_timer_t = gaia::common::timer_t;

TEST(common, retail_assert2)
{
    std::random_device dev;
    std::mt19937_64 rng(dev());
    std::uniform_int_distribution<int32_t> dist(0, 10000);

    constexpr size_t c_num_samples = 1000000;
    int32_t numbers[c_num_samples];

    for (int32_t& number : numbers)
    {
        number = dist(rng);
    }

    auto start = g_timer_t::get_time_point();
    const char* suppini = "aaaaaaaa";

    for (int number : numbers)
    {
        try
        {
            ASSERT_PRECONDITION(number >= 0, suppini);
        }
        catch (retail_assertion_failure& ex)
        {
        }
    }

    auto elapsed = g_timer_t::ns_to_ms(g_timer_t::get_duration(start));
    cout << "Elapsed: " << elapsed << endl;
}
