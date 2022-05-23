////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia/expressions/operators.hpp"
#include "gaia/optional.hpp"

namespace gaia
{
namespace expressions
{

// This file enables SQL-style null propagation on non-comparison operators with
// optional types.
//
// We do not support expressions taking in hardcoded user nullopt values e.g.
// nullopt && expr::field. This is because according to the C++ specification, nullopt
// is not used outside of initialization/comparison of optional.

// Template type for default optional operations.
// We define the types for the non-comparison operators.
// T_left and T_right are the unwrapped optional type.

template <typename T_left, typename T_right>
using and_optional_type = typename common::optional_t<and_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using or_optional_type = typename common::optional_t<or_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using xor_optional_type = typename common::optional_t<xor_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using add_optional_type = typename common::optional_t<add_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using sub_optional_type = typename common::optional_t<sub_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using mul_optional_type = typename common::optional_t<mul_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using div_optional_type = typename common::optional_t<div_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using mod_optional_type = typename common::optional_t<mod_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using band_optional_type = typename common::optional_t<band_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using bor_optional_type = typename common::optional_t<bor_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using shl_optional_type = typename common::optional_t<shl_type<T_left, T_right>>;
template <typename T_left, typename T_right>
using shr_optional_type = typename common::optional_t<shr_type<T_left, T_right>>;

template <typename T_operand>
using neg_optional_type = typename common::optional_t<neg_type<T_operand>>;
template <typename T_operand>
using pos_optional_type = typename common::optional_t<pos_type<T_operand>>;
template <typename T_operand>
using not_optional_type = typename common::optional_t<not_type<T_operand>>;
template <typename T_operand>
using inv_optional_type = typename common::optional_t<inv_type<T_operand>>;

template <typename T_left, typename T_right>
inline constexpr and_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_and_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() && right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr and_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_and_t)
{
    if (left.has_value())
    {
        return left.value() && right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr and_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_and_t)
{
    if (right.has_value())
    {
        return left && right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr or_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_or_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() || right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr or_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_or_t)
{
    if (left.has_value())
    {
        return left.value() || right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr or_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_or_t)
{
    if (right.has_value())
    {
        return left || right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr xor_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_xor_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() ^ right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr xor_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_xor_t)
{
    if (left.has_value())
    {
        return left.value() ^ right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr xor_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_xor_t)
{
    if (right.has_value())
    {
        return left ^ right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr add_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_add_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() + right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr add_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_add_t)
{
    if (left.has_value())
    {
        return left.value() + right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr add_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_add_t)
{
    if (right.has_value())
    {
        return left + right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr sub_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_sub_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() - right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr sub_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_sub_t)
{
    if (left.has_value())
    {
        return left.value() - right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr sub_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_sub_t)
{
    if (right.has_value())
    {
        return left - right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr mul_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_mul_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() * right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr mul_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_mul_t)
{
    if (left.has_value())
    {
        return left.value() * right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr mul_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_mul_t)
{
    if (right.has_value())
    {
        return left * right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr div_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_div_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() / right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr div_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_div_t)
{
    if (left.has_value())
    {
        return left.value() / right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr div_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_div_t)
{
    if (right.has_value())
    {
        return left / right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr mod_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_mod_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() % right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr mod_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_mod_t)
{
    if (left.has_value())
    {
        return left.value() % right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr mod_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_mod_t)
{
    if (right.has_value())
    {
        return left % right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr band_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_band_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() & right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr band_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_band_t)
{
    if (left.has_value())
    {
        return left.value() & right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr band_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_band_t)
{
    if (right.has_value())
    {
        return left & right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr bor_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_bor_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() | right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr bor_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_bor_t)
{
    if (left.has_value())
    {
        return left.value() | right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr bor_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_bor_t)
{
    if (right.has_value())
    {
        return left | right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr shl_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_shl_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() << right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr shl_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_shl_t)
{
    if (left.has_value())
    {
        return left.value() << right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr shl_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_shl_t)
{
    if (right.has_value())
    {
        return left << right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr shr_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const common::optional_t<T_right>& right, operator_shr_t)
{
    if (left.has_value() && right.has_value())
    {
        return left.value() >> right.value();
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr shr_optional_type<T_left, T_right>
evaluate_operator(const common::optional_t<T_left>& left, const T_right& right, operator_shr_t)
{
    if (left.has_value())
    {
        return left.value() >> right;
    }
    return common::nullopt;
}

template <typename T_left, typename T_right>
inline constexpr shr_optional_type<T_left, T_right>
evaluate_operator(const T_left& left, const common::optional_t<T_right>& right, operator_shr_t)
{
    if (right.has_value())
    {
        return left >> right.value();
    }
    return common::nullopt;
}

template <typename T_operand>
inline constexpr not_optional_type<T_operand>
evaluate_operator(const common::optional_t<T_operand>& operand, operator_not_t)
{
    if (operand.has_value())
    {
        return !operand.value();
    }
    return common::nullopt;
}

template <typename T_operand>
inline constexpr neg_optional_type<T_operand>
evaluate_operator(const common::optional_t<T_operand>& operand, operator_neg_t)
{
    if (operand.has_value())
    {
        return -operand.value();
    }
    return common::nullopt;
}

template <typename T_operand>
inline constexpr pos_optional_type<T_operand>
evaluate_operator(const common::optional_t<T_operand>& operand, operator_pos_t)
{
    if (operand.has_value())
    {
        return +operand.value();
    }
    return common::nullopt;
}

template <typename T_operand>
inline constexpr inv_optional_type<T_operand>
evaluate_operator(const common::optional_t<T_operand>& operand, operator_inv_t)
{
    if (operand.has_value())
    {
        return ~operand.value();
    }
    return common::nullopt;
}

} // namespace expressions
} // namespace gaia
