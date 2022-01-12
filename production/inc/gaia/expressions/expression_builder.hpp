/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <type_traits>

#include "expression_ast.hpp"
#include "expression_traits.hpp"

/*
 * This is a generic expression AST builder for non-specialized expression types.
 * The expression would evaluate to the same result and type as what is defined in C++.
 * Operator precedence rules will be identical to C++. Short-circuit evaluation rules are
 * identical to C++.
 *
 * Specialized AST builders exist for certain types such as strings to allow for custom
 * behavior.
 *
 * The builder consists of free-standing operator overloads that will piece expression AST
 * pieces together into an expression tree.
 *
 * Generically operation support for overloading operators for binary expressions:
 * 1) Both operands are expressions.
 * 2) The left operand is a value.
 * 3) The right operand is a value.
 *
 * The case where both expressions of values is notably absent. This is because
 * the C++ compiler already handles this case.
 *
 */

namespace gaia
{
namespace expressions
{

// == operator.
template <typename T_bind, typename T_left, typename T_right>
binary_expression_t<T_bind, eq_type<T_left, T_right>, T_left, T_right, operator_token_t::e_eq>
operator==(
    const expression_t<T_bind, T_left>& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, eq_type<T_left, T_right>, T_left, T_right, operator_token_t::e_eq>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_left>& right_expression) {
            return (*left_expression)(bind) == (*right_expression)(bind);
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_right>::value>>
binary_expression_t<T_bind, eq_type<T_left, T_right>, T_left, T_right, operator_token_t::e_eq>
operator==(
    const expression_t<T_bind, T_left>& left,
    const T_right& right)
{
    return binary_expression_t<T_bind, eq_type<T_left, T_right>, T_left, T_right, operator_token_t::e_eq>(
        left, static_cast<value_accessor_t<T_bind, T_right>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_right>&) {
            return (*left_expression)(bind) == right;
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_left>::value>>
binary_expression_t<T_bind, eq_type<T_left, T_right>, T_left, T_right, operator_token_t::e_eq>
operator==(
    const T_left& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, eq_type<T_left, T_right>, T_left, T_right, operator_token_t::e_eq>(
        static_cast<value_accessor_t<T_bind, T_left>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>&,
            const subexpression_t<T_bind, T_right>& right_expression) {
            return left == (*right_expression)(bind);
        });
}

// != operator.
template <typename T_bind, typename T_left, typename T_right>
binary_expression_t<T_bind, ne_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ne>
operator!=(
    const expression_t<T_bind, T_left>& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, ne_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ne>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_left>& right_expression) {
            return (*left_expression)(bind) != (*right_expression)(bind);
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_right>::value>>
binary_expression_t<T_bind, ne_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ne>
operator!=(
    const expression_t<T_bind, T_left>& left,
    const T_right& right)
{
    return binary_expression_t<T_bind, ne_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ne>(
        left, static_cast<value_accessor_t<T_bind, T_right>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_right>&) {
            return (*left_expression)(bind) != right;
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_left>::value>>
binary_expression_t<T_bind, ne_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ne>
operator!=(
    const T_left& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, ne_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ne>(
        static_cast<value_accessor_t<T_bind, T_left>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>&,
            const subexpression_t<T_bind, T_right>& right_expression) {
            return left != (*right_expression)(bind);
        });
}

// > operator.
template <typename T_bind, typename T_left, typename T_right>
binary_expression_t<T_bind, gt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_gt>
operator>(
    const expression_t<T_bind, T_left>& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, gt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_gt>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_left>& right_expression) {
            return (*left_expression)(bind) > (*right_expression)(bind);
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_right>::value>>
binary_expression_t<T_bind, gt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_gt>
operator>(
    const expression_t<T_bind, T_left>& left,
    const T_right& right)
{
    return binary_expression_t<T_bind, gt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_gt>(
        left, static_cast<value_accessor_t<T_bind, T_right>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_right>&) {
            return (*left_expression)(bind) > right;
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_left>::value>>
binary_expression_t<T_bind, gt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_gt>
operator>(
    const T_left& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, gt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_gt>(
        static_cast<value_accessor_t<T_bind, T_left>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>&,
            const subexpression_t<T_bind, T_right>& right_expression) {
            return left > (*right_expression)(bind);
        });
}

// >= operator.
template <typename T_bind, typename T_left, typename T_right>
binary_expression_t<T_bind, ge_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ge>
operator>=(
    const expression_t<T_bind, T_left>& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, ge_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ge>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_left>& right_expression) {
            return (*left_expression)(bind) >= (*right_expression)(bind);
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_right>::value>>
binary_expression_t<T_bind, ge_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ge>
operator>=(
    const expression_t<T_bind, T_left>& left,
    const T_right& right)
{
    return binary_expression_t<T_bind, ge_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ge>(
        left, static_cast<value_accessor_t<T_bind, T_right>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_right>&) {
            return (*left_expression)(bind) >= right;
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_left>::value>>
binary_expression_t<T_bind, ge_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ge>
operator>=(
    const T_left& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, ge_type<T_left, T_right>, T_left, T_right, operator_token_t::e_ge>(
        static_cast<value_accessor_t<T_bind, T_left>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>&,
            const subexpression_t<T_bind, T_right>& right_expression) {
            return left >= (*right_expression)(bind);
        });
}

// < operator.
template <typename T_bind, typename T_left, typename T_right>
binary_expression_t<T_bind, lt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_lt>
operator<(
    const expression_t<T_bind, T_left>& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, lt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_lt>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_left>& right_expression) {
            return (*left_expression)(bind) < (*right_expression)(bind);
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_right>::value>>
binary_expression_t<T_bind, lt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_lt>
operator<(
    const expression_t<T_bind, T_left>& left,
    const T_right& right)
{
    return binary_expression_t<T_bind, ne_type<T_left, T_right>, T_left, T_right, operator_token_t::e_lt>(
        left, static_cast<value_accessor_t<T_bind, T_right>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_right>&) {
            return (*left_expression)(bind) < right;
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_left>::value>>
binary_expression_t<T_bind, lt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_lt>
operator<(
    const T_left& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, lt_type<T_left, T_right>, T_left, T_right, operator_token_t::e_lt>(
        static_cast<value_accessor_t<T_bind, T_left>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>&,
            const subexpression_t<T_bind, T_right>& right_expression) {
            return left < (*right_expression)(bind);
        });
}

// <= operator.
template <typename T_bind, typename T_left, typename T_right>
binary_expression_t<T_bind, le_type<T_left, T_right>, T_left, T_right, operator_token_t::e_le>
operator<=(
    const expression_t<T_bind, T_left>& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, le_type<T_left, T_right>, T_left, T_right, operator_token_t::e_le>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_left>& right_expression) {
            return (*left_expression)(bind) <= (*right_expression)(bind);
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_right>::value>>
binary_expression_t<T_bind, le_type<T_left, T_right>, T_left, T_right, operator_token_t::e_le>
operator<=(
    const expression_t<T_bind, T_left>& left,
    const T_right& right)
{
    return binary_expression_t<T_bind, or_type<T_left, T_right>, T_left, T_right, operator_token_t::e_le>(
        left, static_cast<value_accessor_t<T_bind, T_right>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_right>&) {
            return (*left_expression)(bind) <= right;
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_left>::value>>
binary_expression_t<T_bind, le_type<T_left, T_right>, T_left, T_right, operator_token_t::e_le>
operator<=(
    const T_left& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, le_type<T_left, T_right>, T_left, T_right, operator_token_t::e_le>(
        static_cast<value_accessor_t<T_bind, T_left>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>&,
            const subexpression_t<T_bind, T_right>& right_expression) {
            return left <= (*right_expression)(bind);
        });
}

// && operator.
template <typename T_bind, typename T_left, typename T_right>
binary_expression_t<T_bind, and_type<T_left, T_right>, T_left, T_right, operator_token_t::e_and>
operator&&(
    const expression_t<T_bind, T_left>& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, and_type<T_left, T_right>, T_left, T_right, operator_token_t::e_and>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_left>& right_expression) {
            return (*left_expression)(bind) && (*right_expression)(bind);
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_right>::value>>
binary_expression_t<T_bind, and_type<T_left, T_right>, T_left, T_right, operator_token_t::e_and>
operator&&(
    const expression_t<T_bind, T_left>& left,
    const T_right& right)
{
    return binary_expression_t<T_bind, and_type<T_left, T_right>, T_left, T_right, operator_token_t::e_and>(
        left, static_cast<value_accessor_t<T_bind, T_right>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_right>&) {
            return (*left_expression)(bind) && right;
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_left>::value>>
binary_expression_t<T_bind, and_type<T_left, T_right>, T_left, T_right, operator_token_t::e_and>
operator&&(
    const T_left& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, or_type<T_left, T_right>, T_left, T_right, operator_token_t::e_and>(
        static_cast<value_accessor_t<T_bind, T_left>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>&,
            const subexpression_t<T_bind, T_right>& right_expression) {
            return left && (*right_expression)(bind);
        });
}

// || operator.
template <typename T_bind, typename T_left, typename T_right>
binary_expression_t<T_bind, or_type<T_left, T_right>, T_left, T_right, operator_token_t::e_or>
operator||(
    const expression_t<T_bind, T_left>& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, or_type<T_left, T_right>, T_left, T_right, operator_token_t::e_or>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_left>& right_expression) {
            return (*left_expression)(bind) || (*right_expression)(bind);
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_right>::value>>
binary_expression_t<T_bind, or_type<T_left, T_right>, T_left, T_right, operator_token_t::e_or>
operator||(
    const expression_t<T_bind, T_left>& left,
    const T_right& right)
{
    return binary_expression_t<T_bind, or_type<T_left, T_right>, T_left, T_right, operator_token_t::e_or>(
        left, static_cast<value_accessor_t<T_bind, T_right>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>& left_expression,
            const subexpression_t<T_bind, T_right>&) {
            return (*left_expression)(bind) || right;
        });
}

template <
    typename T_bind, typename T_left, typename T_right,
    typename T_type_constraint = std::enable_if_t<!is_expression<T_left>::value>>
binary_expression_t<T_bind, or_type<T_left, T_right>, T_left, T_right, operator_token_t::e_or>
operator||(
    const T_left& left,
    const expression_t<T_bind, T_right>& right)
{
    return binary_expression_t<T_bind, or_type<T_left, T_right>, T_left, T_right, operator_token_t::e_or>(
        static_cast<value_accessor_t<T_bind, T_left>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, T_left>&,
            const subexpression_t<T_bind, T_right>& right_expression) {
            return left || (*right_expression)(bind);
        });
}

// ! operator.
template <typename T_bind, typename T_operand>
unary_expression_t<T_bind, not_type<T_operand>, T_operand, operator_token_t::e_not>
operator!(
    const expression_t<T_bind, T_operand>& operand)
{
    return unary_expression_t<T_bind, not_type<T_operand>, T_operand, operator_token_t::e_not>(
        operand, [](const T_bind& bind, const subexpression_t<T_bind, T_operand>& operand_expression) {
            return !(*operand_expression)(bind);
        });
}

} // namespace expressions
} // namespace gaia
