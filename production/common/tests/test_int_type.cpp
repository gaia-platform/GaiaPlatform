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
    constexpr int_type_t<uint16_t> zero;
    constexpr int_type_t<uint16_t> seven(7);
    constexpr int_type_t<uint16_t> nine(9);

    uint16_t value = zero;
    EXPECT_EQ(0, value);

    // Equality.
    value = seven;
    EXPECT_EQ(7, value);
    value = nine;
    EXPECT_EQ(9, value);

    // Comparisons.
    EXPECT_TRUE(seven == seven);
    EXPECT_FALSE(seven == nine);
    EXPECT_TRUE(seven != nine);
    EXPECT_TRUE(seven < nine);
    EXPECT_FALSE(seven > nine);
    EXPECT_TRUE(seven <= seven);
    EXPECT_TRUE(nine >= nine);
    EXPECT_TRUE(seven <= nine);
    EXPECT_FALSE(seven >= nine);

    EXPECT_TRUE(seven == 7);
    EXPECT_FALSE(seven == 9);
    EXPECT_TRUE(seven != 9);
    EXPECT_TRUE(seven < 9);
    EXPECT_FALSE(seven > 9);
    EXPECT_TRUE(seven <= 7);
    EXPECT_TRUE(nine >= 9);
    EXPECT_TRUE(seven <= 9);
    EXPECT_FALSE(seven >= 9);

    EXPECT_TRUE(7 == seven);
    EXPECT_FALSE(7 == nine);
    EXPECT_TRUE(7 != nine);
    EXPECT_TRUE(7 < nine);
    EXPECT_FALSE(7 > nine);
    EXPECT_TRUE(7 <= seven);
    EXPECT_TRUE(9 >= nine);
    EXPECT_TRUE(7 <= nine);
    EXPECT_FALSE(7 >= nine);

    // Addition.
    value = seven + nine;
    EXPECT_EQ(16, value);

    value = seven;
    value += nine;
    EXPECT_EQ(16, value);

    value = 7;
    value += 9;
    EXPECT_EQ(16, value);

    // Multiplication.
    value = seven * nine;
    EXPECT_EQ(63, value);

    value = seven;
    value *= nine;
    EXPECT_EQ(63, value);

    value = 7;
    value *= 9;
    EXPECT_EQ(63, value);

    // Modulo.
    value = nine % seven;
    EXPECT_EQ(2, value);

    value = nine;
    value %= seven;
    EXPECT_EQ(2, value);

    value = 9;
    value %= 7;
    EXPECT_EQ(2, value);

    // Subtraction.
    value = nine - seven;
    EXPECT_EQ(2, value);

    value = nine;
    value -= seven;
    EXPECT_EQ(2, value);

    value = 9;
    value -= 7;
    EXPECT_EQ(2, value);

    // Division.
    value = nine / seven;
    EXPECT_EQ(1, value);

    value = nine;
    value /= seven;
    EXPECT_EQ(1, value);

    value = 9;
    value /= 7;
    EXPECT_EQ(1, value);

    // Maximum value.
    int_type_t<uint16_t> max(-1);
    value = max;
    EXPECT_EQ(numeric_limits<uint16_t>::max(), value);
}
