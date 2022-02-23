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

    int64_t i64_2 = 100;
    auto i64_value_accessor2 = value_accessor_t<context_t, int64_t>(i64_2);

    auto add_expression = i64_value_accessor + 100;
    ASSERT_EQ(add_expression(g_bind), i64 + 100);

    // yoda
    auto add_expression2 = 100 + i64_value_accessor;
    ASSERT_EQ(add_expression2(g_bind), 100 + i64);

    auto add_expression3 = i64_value_accessor + i64_value_accessor2;
    ASSERT_EQ(add_expression3(g_bind), i64 + i64_2);
}

TEST_F(test_expressions_no_dac, expr_sub)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    int64_t i64_2 = 100;
    auto i64_value_accessor2 = value_accessor_t<context_t, int64_t>(i64_2);

    auto sub_expression = i64_value_accessor - 100;
    ASSERT_EQ(sub_expression(g_bind), i64 - 100);

    auto sub_expression2 = 100 - i64_value_accessor;
    ASSERT_EQ(sub_expression2(g_bind), 100 - i64);

    auto sub_expression3 = i64_value_accessor - i64_value_accessor2;
    ASSERT_EQ(sub_expression3(g_bind), i64 - i64_2);
}

TEST_F(test_expressions_no_dac, expr_mul)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    int64_t i64_2 = 100;
    auto i64_value_accessor2 = value_accessor_t<context_t, int64_t>(i64_2);

    auto mul_expression = i64_value_accessor * 2;
    ASSERT_EQ(mul_expression(g_bind), i64 * 2);

    auto mul_expression2 = 0.25 * i64_value_accessor;
    ASSERT_EQ(mul_expression2(g_bind), 0.25 * i64);

    auto mul_expression3 = i64_value_accessor * i64_value_accessor2;
    ASSERT_EQ(mul_expression3(g_bind), i64 * i64_2);
}

TEST_F(test_expressions_no_dac, expr_div)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    int64_t i64_2 = 100;
    auto i64_value_accessor2 = value_accessor_t<context_t, int64_t>(i64_2);

    auto div_expression = i64_value_accessor / 2;
    ASSERT_EQ(div_expression(g_bind), i64 / 2);

    auto div_expression2 = 2 / i64_value_accessor;
    ASSERT_EQ(div_expression2(g_bind), 2 / i64);

    auto mul_expression3 = i64_value_accessor / i64_value_accessor2;
    ASSERT_EQ(mul_expression3(g_bind), i64 / i64_2);
}

TEST_F(test_expressions_no_dac, expr_mod)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    int64_t i64_2 = 100;
    auto i64_value_accessor2 = value_accessor_t<context_t, int64_t>(i64_2);

    auto mod_expression = i64_value_accessor % 2;
    ASSERT_EQ(mod_expression(g_bind), i64 % 2);

    auto mod_expression2 = 2 % i64_value_accessor;
    ASSERT_EQ(mod_expression2(g_bind), 2 % i64);

    auto mod_expression3 = i64_value_accessor % i64_value_accessor2;
    ASSERT_EQ(mod_expression3(g_bind), i64 % i64_2);
}

TEST_F(test_expressions_no_dac, expr_band)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    int64_t i64_2 = 100;
    auto i64_value_accessor2 = value_accessor_t<context_t, int64_t>(i64_2);

    auto band_expression = i64_value_accessor & 1;
    ASSERT_EQ(band_expression(g_bind), i64 & 1);

    auto band_expression2 = 2 & i64_value_accessor;
    ASSERT_EQ(band_expression2(g_bind), 2 & i64);

    auto band_expression3 = i64_value_accessor & i64_value_accessor2;
    ASSERT_EQ(band_expression3(g_bind), i64 & i64_2);
}

TEST_F(test_expressions_no_dac, expr_bor)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    int64_t i64_2 = 100;
    auto i64_value_accessor2 = value_accessor_t<context_t, int64_t>(i64_2);

    auto bor_expression = i64_value_accessor | 1;
    ASSERT_EQ(bor_expression(g_bind), i64 | 1);

    auto bor_expression2 = 2 | i64_value_accessor;
    ASSERT_EQ(bor_expression2(g_bind), 2 | i64);

    auto bor_expression3 = i64_value_accessor | i64_value_accessor2;
    ASSERT_EQ(bor_expression3(g_bind), i64 | i64_2);
}

TEST_F(test_expressions_no_dac, expr_shl)
{
    int64_t i64 = 10;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    int64_t i64_2 = 2;
    auto i64_value_accessor2 = value_accessor_t<context_t, int64_t>(i64_2);

    auto shl_expression = i64_value_accessor >> 1;
    ASSERT_EQ(shl_expression(g_bind), i64 >> 1);

    auto shl_expression2 = 2 >> i64_value_accessor;
    ASSERT_EQ(shl_expression2(g_bind), 2 >> i64);

    auto shl_expression3 = i64_value_accessor >> i64_value_accessor2;
    ASSERT_EQ(shl_expression3(g_bind), i64 >> i64_2);
}

TEST_F(test_expressions_no_dac, expr_shr)
{
    int64_t i64 = 10;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    int64_t i64_2 = 2;
    auto i64_value_accessor2 = value_accessor_t<context_t, int64_t>(i64_2);

    auto shr_expression = i64_value_accessor << 1;
    ASSERT_EQ(shr_expression(g_bind), i64 << 1);

    auto shr_expression2 = 2 << i64_value_accessor;
    ASSERT_EQ(shr_expression2(g_bind), 2 << i64);

    auto shr_expression3 = i64_value_accessor << i64_value_accessor2;
    ASSERT_EQ(shr_expression3(g_bind), i64 << i64_2);
}

TEST_F(test_expressions_no_dac, expr_neg)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto neg_expression = -i64_value_accessor;
    ASSERT_EQ(neg_expression(g_bind), -i64);
}

TEST_F(test_expressions_no_dac, expr_pos)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto pos_expression = +i64_value_accessor;
    ASSERT_EQ(pos_expression(g_bind), +i64);
}

TEST_F(test_expressions_no_dac, expr_inv)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto inv_expression = ~i64_value_accessor;
    ASSERT_EQ(inv_expression(g_bind), ~i64);
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
