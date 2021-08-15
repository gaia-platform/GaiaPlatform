/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia_internal/common/assert.hpp"

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
