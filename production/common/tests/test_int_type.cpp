/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdint>

#include "gtest/gtest.h"

#include "gaia/int_type.hpp"

using namespace std;
using namespace gaia::common;

template <typename T>
void test_int_type()
{
    constexpr int_type_t<T, 0> zero;
    constexpr int_type_t<T, 0> seven(7);
    constexpr int_type_t<T, 0> nine(9);

    T value = zero;
    EXPECT_EQ(0, value);

    // Equality.
    value = seven;
    EXPECT_EQ(7, value);
    EXPECT_EQ(7, seven.value());
    value = nine;
    EXPECT_EQ(9, value);
    EXPECT_EQ(9, nine.value());

    // -1.
    T expected_result = -1;
    int_type_t<T, 0> negative_one(-1);
    EXPECT_EQ(expected_result, negative_one.value());

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

    int_type_t<T, 0> result;

    // Addition.
    expected_result = 7 + 9;
    result = seven + nine;
    EXPECT_EQ(expected_result, result);

    result = seven;
    result += nine;
    EXPECT_EQ(expected_result, result);

    result = 7;
    result += 9;
    EXPECT_EQ(expected_result, result);

    // Multiplication.
    expected_result = 7 * 9;
    result = seven * nine;
    EXPECT_EQ(expected_result, result);

    result = seven;
    result *= nine;
    EXPECT_EQ(expected_result, result);

    result = 7;
    result *= 9;
    EXPECT_EQ(expected_result, result);

    // Modulo.
    expected_result = 9 % 7;
    result = nine % seven;
    EXPECT_EQ(expected_result, result);

    result = nine;
    result %= seven;
    EXPECT_EQ(expected_result, result);

    result = 9;
    result %= 7;
    EXPECT_EQ(expected_result, result);

    // Subtraction.
    expected_result = 7 - 9;
    result = seven - nine;
    EXPECT_EQ(expected_result, result);

    result = seven;
    result -= nine;
    EXPECT_EQ(expected_result, result);

    result = 7;
    result -= 9;
    EXPECT_EQ(expected_result, result);

    // Division.
    expected_result = 9 / 7;
    result = nine / seven;
    EXPECT_EQ(expected_result, result);

    result = nine;
    result /= seven;
    EXPECT_EQ(expected_result, result);

    result = 9;
    result /= 7;
    EXPECT_EQ(expected_result, result);

    // Post-increment.
    int_type_t<T, 0> data = nine;
    result = data++;
    expected_result = 9;
    EXPECT_EQ(expected_result, result);
    expected_result++;
    EXPECT_EQ(expected_result, data);

    // Pre-increment.
    data = nine;
    result = ++data;
    expected_result = 9;
    ++expected_result;
    EXPECT_EQ(expected_result, result);
    EXPECT_EQ(expected_result, data);

    // Post-decrement.
    data = nine;
    result = data--;
    expected_result = 9;
    EXPECT_EQ(expected_result, result);
    expected_result--;
    EXPECT_EQ(expected_result, data);

    // Pre-decrement.
    data = nine;
    result = --data;
    expected_result = 9;
    --expected_result;
    EXPECT_EQ(expected_result, result);
    EXPECT_EQ(expected_result, data);

    // Direct updating.
    data = nine;
    data.value_ref() = seven.value();
    expected_result = seven;
    EXPECT_EQ(expected_result, data);
}

TEST(common, int_type)
{
    test_int_type<uint8_t>();
    test_int_type<uint16_t>();
    test_int_type<uint32_t>();
    test_int_type<uint64_t>();

    test_int_type<int8_t>();
    test_int_type<int16_t>();
    test_int_type<int32_t>();
    test_int_type<int64_t>();
}

TEST(common, int_type_bool_cast)
{
    constexpr uint64_t c_uint64_max = std::numeric_limits<uint64_t>::max();

    // Start with an uninitialized value.
    // It should be interpreted as 'false'.
    // But if we don't use an explicit cast, the value
    // will be neither true nor false.
    int_type_t<uint64_t, c_uint64_max> value;
    EXPECT_EQ(c_uint64_max, value.value());
    EXPECT_EQ(false, static_cast<bool>(value));
    EXPECT_EQ(false, !!value);
    EXPECT_NE(true, static_cast<bool>(value));
    EXPECT_NE(true, !!value);
    EXPECT_NE(false, value);
    EXPECT_NE(true, value);

    // Initialize our value.
    // It should now be interpreted as 'true'.
    // But again, if we don't use an explicit cast, the value
    // will be neither true nor false.
    value = 7;
    EXPECT_NE(c_uint64_max, value.value());
    EXPECT_EQ(true, static_cast<bool>(value));
    EXPECT_EQ(true, !!value);
    EXPECT_NE(false, static_cast<bool>(value));
    EXPECT_NE(false, !!value);
    EXPECT_NE(false, value);
    EXPECT_NE(true, value);
}
