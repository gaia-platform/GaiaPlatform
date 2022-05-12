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

TEST(common__assert__test, regular)
{
    try
    {
        ASSERT_INVARIANT(true, "Unexpected triggering of assert!");
        SUCCEED() << "No exception should have been thrown.";
    }
    catch (const std::exception& e)
    {
        FAIL() << "Did not expect an exception!";
    }

    try
    {
        ASSERT_INVARIANT(false, "Expected triggering of assert.");
        FAIL() << "Did not expect to get here!";
    }
    catch (const std::exception& e)
    {
        SUCCEED() << "An exception was thrown, as expected.";
        cerr << "Exception message: " << e.what() << '\n';
    }
}

TEST(common__assert__test, debug)
{
    try
    {
        DEBUG_ASSERT_INVARIANT(true, "Unexpected triggering of debug assert!");
        SUCCEED() << "No exception should have been thrown.";
    }
    catch (const std::exception& e)
    {
        FAIL() << "Did not expect an exception!";
    }

    try
    {
        DEBUG_ASSERT_INVARIANT(false, "Expected (in debug builds only) triggering of debug assert.");
#ifdef DEBUG
        FAIL() << "Did not expect to get here!";
#else
        SUCCEED() << "No exception should have been thrown when not in debug mode.";
#endif
    }
    catch (const std::exception& e)
    {
#ifdef DEBUG
        SUCCEED() << "An exception was thrown, as expected.";
#else
        FAIL() << "Did not expect an exception when not in debug mode!";
#endif
        cerr << "Exception message: " << e.what() << '\n';
    }
}

using g_timer_t = gaia::common::timer_t;

inline int32_t check_number(int32_t number)
{
    ASSERT_PRECONDITION(
        number >= 0,
        "check_number() was called with a negative number!");

    return number + 1;
}

inline int32_t check_number_format(int32_t number)
{
    ASSERT_PRECONDITION(
        number >= 0,
        gaia_fmt::format("check_number() was called with a negative number: {}!", number).c_str());

    return number + 1;
}

inline int32_t check_number_debug(int32_t number)
{
    DEBUG_ASSERT_PRECONDITION(
        number >= 0,
        gaia_fmt::format("check_number() was called with a negative number: {}!", number).c_str());

    return number + 1;
}

TEST(common__assert__test, perf)
{
    // Increase these values when using the test as a performance test.
    // The number of iterations can be increased
    // once the number of samples no longer fits in memory.
    constexpr size_t c_count_numbers = 10;
    constexpr size_t c_count_iterations = 1;

    constexpr int32_t c_min_distribution_value = 0;
    constexpr int32_t c_max_distribution_value = 100000;
    std::random_device dev;
    std::mt19937_64 rng(dev());
    std::uniform_int_distribution<int32_t> dist(c_min_distribution_value, c_max_distribution_value);

    // Pre-generate the random input values.
    int32_t numbers[c_count_numbers];
    for (int32_t& number : numbers)
    {
        number = dist(rng);
    }

    cout << endl;

    // Test debug asserts.
    auto start = g_timer_t::get_time_point();

    for (size_t i = 0; i < c_count_iterations; ++i)
    {
        for (int32_t number : numbers)
        {
            check_number_debug(number);
        }
    }

    double elapsed = g_timer_t::get_duration(start);

    cout << ">>> Time Elapsed (helper with DEBUG_ASSERT): " << g_timer_t::ns_to_ms(elapsed) << "ms." << endl;
    cout << ">>> Average time: " << elapsed / (c_count_numbers * c_count_iterations) << "ns." << endl;

    // Test the assert condition itself.
    start = g_timer_t::get_time_point();

    size_t count_condition_failures = 0;
    for (size_t i = 0; i < c_count_iterations; ++i)
    {
        for (int32_t number : numbers)
        {
            if (number < 0)
            {
                ++count_condition_failures;
            }
        }
    }

    ASSERT_EQ(0, count_condition_failures);

    elapsed = g_timer_t::get_duration(start);

    cout << ">>> Time Elapsed (condition): " << g_timer_t::ns_to_ms(elapsed) << "ms." << endl;
    cout << ">>> Average time: " << elapsed / (c_count_numbers * c_count_iterations) << "ns." << endl;

    // Test regular asserts with a formatted assertion message.
    start = g_timer_t::get_time_point();

    for (size_t i = 0; i < c_count_iterations; ++i)
    {
        for (int32_t number : numbers)
        {
            check_number_format(number);
        }
    }

    elapsed = g_timer_t::get_duration(start);

    cout << ">>> Time Elapsed (helper with gaia_fmt::format): " << g_timer_t::ns_to_ms(elapsed) << "ms." << endl;
    cout << ">>> Average time: " << elapsed / (c_count_numbers * c_count_iterations) << "ns." << endl;

    // Test regular asserts with a static assertion message.
    start = g_timer_t::get_time_point();

    for (size_t i = 0; i < c_count_iterations; ++i)
    {
        for (int32_t number : numbers)
        {
            check_number(number);
        }
    }

    elapsed = g_timer_t::get_duration(start);

    cout << ">>> Time Elapsed (helper with static message): " << g_timer_t::ns_to_ms(elapsed) << "ms." << endl;
    cout << ">>> Average time: " << elapsed / (c_count_numbers * c_count_iterations) << "ns." << endl;

    // Test inline asserts with a static assertion message.
    start = g_timer_t::get_time_point();

    for (size_t i = 0; i < c_count_iterations; ++i)
    {
        for (int32_t number : numbers)
        {
            ASSERT_PRECONDITION(
                number >= 0,
                "check_number() was called with a negative number!");
        }
    }

    elapsed = g_timer_t::get_duration(start);

    cout << ">>> Time Elapsed (inline with static message): " << g_timer_t::ns_to_ms(elapsed) << "ms." << endl;
    cout << ">>> Average time: " << elapsed / (c_count_numbers * c_count_iterations) << "ns." << endl;

    cout << endl;
}
