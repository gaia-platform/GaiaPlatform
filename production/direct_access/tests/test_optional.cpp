/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/direct_access/dac_optional.hpp"

using namespace gaia::direct_access;

class dac_optional_test : public ::testing::Test
{
protected:
    dac_optional_test() = default;
};

template <typename T_value>
void test_optional_value(T_value value, T_value other_value)
{
    optional_t<T_value> opt = value;
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(opt.value(), value);
    ASSERT_EQ(*opt, value);
    ASSERT_EQ(opt.value_or(other_value), value);

    const optional_t<T_value> const_opt = value;
    ASSERT_TRUE(const_opt.has_value());
    ASSERT_EQ(const_opt.value(), value);
    ASSERT_EQ(*const_opt, value);
    ASSERT_EQ(const_opt.value_or(other_value), value);

    optional_t<T_value> empty_opt;
    ASSERT_FALSE(empty_opt.has_value());
    ASSERT_EQ(empty_opt.value_or(other_value), other_value);
    ASSERT_THROW(empty_opt.value(), optional_value_not_found);
    ASSERT_THROW(*empty_opt, optional_value_not_found);

    opt.reset();
    ASSERT_FALSE(opt.has_value());
    ASSERT_EQ(opt.value_or(other_value), other_value);
    ASSERT_THROW(opt.value(), optional_value_not_found);
    ASSERT_THROW(*opt, optional_value_not_found);
}

TEST_F(dac_optional_test, test_optional_int)
{
    test_optional_value<int8_t>(3, 5);
    test_optional_value<int16_t>(3, 5);
    test_optional_value<int32_t>(3, 5);
    test_optional_value<int64_t>(3, 5);

    test_optional_value<uint8_t>(3, 5);
    test_optional_value<uint16_t>(3, 5);
    test_optional_value<uint32_t>(3, 5);
    test_optional_value<uint64_t>(3, 5);
}

TEST_F(dac_optional_test, test_optional_fp)
{
    test_optional_value<float>(3.0f, 5.0f);
    test_optional_value<double>(3.0, 5.0);
    test_optional_value<long double>(3.0, 5.0);
}

TEST_F(dac_optional_test, test_optional_bool)
{
    test_optional_value<bool>(true, false);
}
