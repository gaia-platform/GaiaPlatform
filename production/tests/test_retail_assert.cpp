/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "retail_assert.hpp"

using namespace std;
using namespace gaia::common;

TEST(common, retail_assert)
{
    try
    {
        retail_assert(true, "Unexpected triggering of retail assert!");
        EXPECT_EQ(true, true);
    }
    catch(const std::exception& e)
    {
        EXPECT_EQ(true, false);
    }

    try
    {
        retail_assert(false, "Expected triggering of retail assert.");
        EXPECT_EQ(true, false);
    }
    catch(const std::exception& e)
    {
        EXPECT_EQ(true, true);
        cerr << "Exception message: " << e.what() << '\n';
    }
}
