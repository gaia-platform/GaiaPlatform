/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/expressions/expressions.hpp"

using namespace gaia::expressions;

class test_expressions_no_dac : public ::testing::Test
{
};

// Make a fake context to bind the expressions against.
struct context_t
{
} g_bind;

TEST_F(test_expressions_no_dac, expr_add)
{

    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto add_expression = i64_value_accessor + 100;
    ASSERT_EQ(add_expression(g_bind), i64 + 100);

    // yoda
    auto add_expression2 = 100 + i64_value_accessor;
    ASSERT_EQ(add_expression2(g_bind), 100 + i64);
}

TEST_F(test_expressions_no_dac, expr_sub)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto sub_expression = i64_value_accessor - 100;
    ASSERT_EQ(sub_expression(g_bind), i64 - 100);

    auto sub_expression2 = 100 - i64_value_accessor;
    ASSERT_EQ(sub_expression2(g_bind), 100 - i64);
}

TEST_F(test_expressions_no_dac, expr_mul)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto mul_expression = i64_value_accessor * 2;
    ASSERT_EQ(mul_expression(g_bind), i64 * 2);

    auto mul_expression2 = 0.25 * i64_value_accessor;
    ASSERT_EQ(mul_expression2(g_bind), 0.25 * i64);
}

TEST_F(test_expressions_no_dac, expr_div)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto div_expression = i64_value_accessor / 2;
    ASSERT_EQ(div_expression(g_bind), i64 / 2);

    auto div_expression2 = 2 / i64_value_accessor;
    ASSERT_EQ(div_expression2(g_bind), 2 / i64);
}

TEST_F(test_expressions_no_dac, expr_neg)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto neg_expression = -i64_value_accessor;
    ASSERT_EQ(neg_expression(g_bind), -i64);
}

TEST_F(test_expressions_no_dac, expr_avg)
{
    int64_t i1 = 500;
    auto value1 = value_accessor_t<context_t, int64_t>(i1);

    int64_t i2 = 400;
    auto value2 = value_accessor_t<context_t, int64_t>(i2);

    int64_t i3 = 300;
    auto value3 = value_accessor_t<context_t, int64_t>(i3);

    auto avg_expression = (value1 + value2 + value3) / 3.0;
    ASSERT_EQ(avg_expression(g_bind), (i1 + i2 + i3) / 3.0);
}
