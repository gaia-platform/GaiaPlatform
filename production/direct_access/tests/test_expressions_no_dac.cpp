////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/expressions/expressions.hpp"
#include "gaia/optional.hpp"

using namespace gaia::expressions;
using namespace gaia::common;

class direct_access__expressions_no_dac__test : public ::testing::Test
{
};

// Make a fake context to bind the expressions against.
struct context_t
{
} g_bind;

TEST_F(direct_access__expressions_no_dac__test, add)
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

TEST_F(direct_access__expressions_no_dac__test, sub)
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

TEST_F(direct_access__expressions_no_dac__test, mul)
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

TEST_F(direct_access__expressions_no_dac__test, div)
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

TEST_F(direct_access__expressions_no_dac__test, mod)
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

TEST_F(direct_access__expressions_no_dac__test, band)
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

TEST_F(direct_access__expressions_no_dac__test, bor)
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

TEST_F(direct_access__expressions_no_dac__test, shl)
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

TEST_F(direct_access__expressions_no_dac__test, shr)
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

TEST_F(direct_access__expressions_no_dac__test, neg)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto neg_expression = -i64_value_accessor;
    ASSERT_EQ(neg_expression(g_bind), -i64);
}

TEST_F(direct_access__expressions_no_dac__test, pos)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto pos_expression = +i64_value_accessor;
    ASSERT_EQ(pos_expression(g_bind), +i64);
}

TEST_F(direct_access__expressions_no_dac__test, inv)
{
    int64_t i64 = 500;
    auto i64_value_accessor = value_accessor_t<context_t, int64_t>(i64);

    auto inv_expression = ~i64_value_accessor;
    ASSERT_EQ(inv_expression(g_bind), ~i64);
}

TEST_F(direct_access__expressions_no_dac__test, avg)
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

TEST_F(direct_access__expressions_no_dac__test, optional_and)
{
    optional_t<bool> true_value = true;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<bool>>(true_value);

    optional_t<bool> false_value = false;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<bool>>(false_value);

    optional_t<bool> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<bool>>(no_value);

    auto and_expression = value_accessor1 && false;
    ASSERT_EQ(and_expression(g_bind), true_value.value() && false);

    auto and_expression2 = false && value_accessor1;
    ASSERT_EQ(and_expression2(g_bind), false && true_value.value());

    auto and_expression3 = value_accessor2 && value_accessor1;
    ASSERT_EQ(and_expression3(g_bind), false_value.value() && true_value.value());

    auto and_expression4 = value_accessor2 && value_accessor_no;
    ASSERT_EQ(and_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_or)
{
    optional_t<bool> true_value = true;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<bool>>(true_value);

    optional_t<bool> false_value = false;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<bool>>(false_value);

    optional_t<bool> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<bool>>(no_value);

    auto or_expression = value_accessor1 || false;
    ASSERT_EQ(or_expression(g_bind), true_value.value() || false);

    auto or_expression2 = false || value_accessor1;
    ASSERT_EQ(or_expression2(g_bind), false || true_value.value());

    auto or_expression3 = value_accessor2 || value_accessor1;
    ASSERT_EQ(or_expression3(g_bind), false_value.value() || true_value.value());

    auto or_expression4 = value_accessor2 || value_accessor_no;
    ASSERT_EQ(or_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_xor)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto xor_expression = value_accessor1 ^ 5;
    ASSERT_EQ(xor_expression(g_bind), value1.value() ^ 5);

    auto xor_expression2 = 5 ^ value_accessor1;
    ASSERT_EQ(xor_expression2(g_bind), 5 ^ value1.value());

    auto xor_expression3 = value_accessor2 ^ value_accessor1;
    ASSERT_EQ(xor_expression3(g_bind), value2.value() ^ value1.value());

    auto xor_expression4 = value_accessor2 ^ value_accessor_no;
    ASSERT_EQ(xor_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_add)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto add_expression = value_accessor1 + 5;
    ASSERT_EQ(add_expression(g_bind), value1.value() + 5);

    auto add_expression2 = 5 + value_accessor1;
    ASSERT_EQ(add_expression2(g_bind), 5 + value1.value());

    auto add_expression3 = value_accessor2 + value_accessor1;
    ASSERT_EQ(add_expression3(g_bind), value2.value() + value1.value());

    auto add_expression4 = value_accessor2 + value_accessor_no;
    ASSERT_EQ(add_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_sub)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto sub_expression = value_accessor1 - 5;
    ASSERT_EQ(sub_expression(g_bind), value1.value() - 5);

    auto sub_expression2 = 5 - value_accessor1;
    ASSERT_EQ(sub_expression2(g_bind), 5 - value1.value());

    auto sub_expression3 = value_accessor2 - value_accessor1;
    ASSERT_EQ(sub_expression3(g_bind), value2.value() - value1.value());

    auto sub_expression4 = value_accessor2 - value_accessor_no;
    ASSERT_EQ(sub_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_mul)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto mul_expression = value_accessor1 * 5;
    ASSERT_EQ(mul_expression(g_bind), value1.value() * 5);

    auto mul_expression2 = 5 * value_accessor1;
    ASSERT_EQ(mul_expression2(g_bind), 5 * value1.value());

    auto mul_expression3 = value_accessor2 * value_accessor1;
    ASSERT_EQ(mul_expression3(g_bind), value2.value() * value1.value());

    auto mul_expression4 = value_accessor2 * value_accessor_no;
    ASSERT_EQ(mul_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_div)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto div_expression = value_accessor1 / 5;
    ASSERT_EQ(div_expression(g_bind), value1.value() / 5);

    auto div_expression2 = 5 / value_accessor1;
    ASSERT_EQ(div_expression2(g_bind), 5 / value1.value());

    auto div_expression3 = value_accessor2 / value_accessor1;
    ASSERT_EQ(div_expression3(g_bind), value2.value() / value1.value());

    auto div_expression4 = value_accessor2 / value_accessor_no;
    ASSERT_EQ(div_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_mod)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto mod_expression = value_accessor1 % 5;
    ASSERT_EQ(mod_expression(g_bind), value1.value() % 5);

    auto mod_expression2 = 5 % value_accessor1;
    ASSERT_EQ(mod_expression2(g_bind), 5 % value1.value());

    auto mod_expression3 = value_accessor2 % value_accessor1;
    ASSERT_EQ(mod_expression3(g_bind), value2.value() % value1.value());

    auto mod_expression4 = value_accessor2 % value_accessor_no;
    ASSERT_EQ(mod_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_band)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto band_expression = value_accessor1 & 5;
    ASSERT_EQ(band_expression(g_bind), value1.value() & 5);

    auto band_expression2 = 5 & value_accessor1;
    ASSERT_EQ(band_expression2(g_bind), 5 & value1.value());

    auto band_expression3 = value_accessor2 & value_accessor1;
    ASSERT_EQ(band_expression3(g_bind), value2.value() & value1.value());

    auto band_expression4 = value_accessor2 & value_accessor_no;
    ASSERT_EQ(band_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_bor)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto bor_expression = value_accessor1 | 5;
    ASSERT_EQ(bor_expression(g_bind), value1.value() | 5);

    auto bor_expression2 = 5 | value_accessor1;
    ASSERT_EQ(bor_expression2(g_bind), 5 | value1.value());

    auto bor_expression3 = value_accessor2 | value_accessor1;
    ASSERT_EQ(bor_expression3(g_bind), value2.value() | value1.value());

    auto bor_expression4 = value_accessor2 | value_accessor_no;
    ASSERT_EQ(bor_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_shl)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto shl_expression = value_accessor1 << 5;
    ASSERT_EQ(shl_expression(g_bind), value1.value() << 5);

    auto shl_expression2 = 5 << value_accessor1;
    ASSERT_EQ(shl_expression2(g_bind), 5 << value1.value());

    auto shl_expression3 = value_accessor2 << value_accessor1;
    ASSERT_EQ(shl_expression3(g_bind), value2.value() << value1.value());

    auto shl_expression4 = value_accessor2 << value_accessor_no;
    ASSERT_EQ(shl_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_shr)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);

    optional_t<int32_t> value2 = 2;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);

    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto shr_expression = value_accessor1 >> 5;
    ASSERT_EQ(shr_expression(g_bind), value1.value() >> 5);

    auto shr_expression2 = 5 >> value_accessor1;
    ASSERT_EQ(shr_expression2(g_bind), 5 >> value1.value());

    auto shr_expression3 = value_accessor2 >> value_accessor1;
    ASSERT_EQ(shr_expression3(g_bind), value2.value() >> value1.value());

    auto shr_expression4 = value_accessor2 >> value_accessor_no;
    ASSERT_EQ(shr_expression4(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_not)
{
    optional_t<bool> true_value = true;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<bool>>(true_value);
    optional_t<bool> no_value;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<bool>>(no_value);

    auto not_expression = !value_accessor1;
    ASSERT_EQ(not_expression(g_bind), !true_value.value());

    auto not_expression2 = !value_accessor2;
    ASSERT_EQ(not_expression2(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_neg)
{
    optional_t<int32_t> value = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value);
    optional_t<int32_t> no_value;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto neg_expression = -value_accessor1;
    ASSERT_EQ(neg_expression(g_bind), -value.value());

    auto neg_expression2 = -value_accessor2;
    ASSERT_EQ(neg_expression2(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_pos)
{
    optional_t<int32_t> value = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value);
    optional_t<int32_t> no_value;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto pos_expression = +value_accessor1;
    ASSERT_EQ(pos_expression(g_bind), +value.value());

    auto pos_expression2 = +value_accessor2;
    ASSERT_EQ(pos_expression2(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_inv)
{
    optional_t<int32_t> value = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value);
    optional_t<int32_t> no_value;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto inv_expression = ~value_accessor1;
    ASSERT_EQ(inv_expression(g_bind), ~value.value());

    auto inv_expression2 = ~value_accessor2;
    ASSERT_EQ(inv_expression2(g_bind), nullopt);
}

TEST_F(direct_access__expressions_no_dac__test, optional_avg)
{
    optional_t<int32_t> value1 = 10;
    auto value_accessor1 = value_accessor_t<context_t, optional_t<int32_t>>(value1);
    optional_t<int32_t> value2 = 20;
    auto value_accessor2 = value_accessor_t<context_t, optional_t<int32_t>>(value2);
    optional_t<int32_t> value3 = 30;
    auto value_accessor3 = value_accessor_t<context_t, optional_t<int32_t>>(value3);
    optional_t<int32_t> no_value;
    auto value_accessor_no = value_accessor_t<context_t, optional_t<int32_t>>(no_value);

    auto avg_expression = (value_accessor1 + value_accessor2 + value_accessor3) / 3.0;
    ASSERT_EQ(avg_expression(g_bind), (value1.value() + value2.value() + value3.value()) / 3.0);

    auto avg_expression2 = (value_accessor1 + value_accessor_no + value_accessor2 + value_accessor3) / 4.0;
    ASSERT_EQ(avg_expression2(g_bind), nullopt);
}
