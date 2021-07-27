/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdint>

#include "gtest/gtest.h"

#include "gaia_internal/common/int_type.hpp"

using namespace std;
using namespace gaia::common;

TEST(common, int_type)
{
    int_type_t<uint16_t> zero;

    uint16_t value = zero;
    EXPECT_EQ(0, value);

    int_type_t<uint16_t> seven(7);
    int_type_t<uint16_t> nine(9);

    value = seven;
    EXPECT_EQ(7, value);
    value = nine;
    EXPECT_EQ(9, value);

    int_type_t<uint16_t> sum = seven + nine;

    value = sum;
    EXPECT_EQ(16, value);

    int_type_t<uint16_t> product = seven * nine;

    value = product;
    EXPECT_EQ(63, value);

    int_type_t<uint16_t> modulo = nine % seven;

    value = modulo;
    EXPECT_EQ(2, value);

    EXPECT_EQ(false, nine == seven);
    EXPECT_EQ(true, nine != seven);

    int_type_t<uint16_t> max(-1);

    value = max;
    EXPECT_EQ(numeric_limits<uint16_t>::max(), max);
}
