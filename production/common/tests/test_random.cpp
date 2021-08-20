/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cctype>

#include "gtest/gtest.h"

#include "gaia_internal/common/random.hpp"

TEST(random_test, gen_random_num)
{
    constexpr int c_lower_range = 0;
    constexpr int c_high_range = 15;
    constexpr int c_iterations = (c_high_range - c_lower_range) * 10;

    for (int i = 0; i < c_iterations; ++i)
    {
        int rnd = gaia::common::gen_random_num(c_lower_range, c_high_range);
        ASSERT_TRUE(rnd >= c_lower_range && rnd <= c_high_range)
            << "The number " << rnd << " is outside of the expected range ("
            << c_lower_range << ", " << c_high_range << ")";
    }
}

TEST(random_test, gen_random_str)
{
    constexpr int c_min_str_len = 5;
    constexpr int c_max_str_len = 30;

    for (int i = c_min_str_len; i < c_max_str_len; ++i)
    {
        std::string rnd = gaia::common::gen_random_str(i);

        ASSERT_EQ(rnd.size(), i);

        for (char j : rnd)
        {
            ASSERT_TRUE(::isalnum(j)) << j << " is not alphanumeric";
        }
    }
}
