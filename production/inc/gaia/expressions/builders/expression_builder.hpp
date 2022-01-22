/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <type_traits>

#include "gaia/expressions/builders/expression_bind.hpp"
#include "gaia/expressions/expression_ast.hpp"
#include "gaia/expressions/expression_traits.hpp"
#include "gaia/expressions/operators.hpp"

// This is a generic expression AST builder for expression types.
// The expression would evaluate to the same result and type as what is defined in C++.
// Operator precedence rules will be identical to C++. Short-circuit evaluation rules are
// identical to C++.
//
// Specialized operator evaluations exist for certain types such as strings to allow for custom
// behavior.
//
// The builder consists of free-standing operator overloads that will piece the expression AST
// pieces together into an expression tree.
//
// Generically operation support for overloading operators for binary expressions:
// 1) Both operands are expressions.
// 2) The left operand is a value.
// 3) The right operand is a value.
//
// The case where both operands are values is notably absent. This is because
// the C++ compiler already natively handles this case!

namespace gaia
{
namespace expressions
{

// == operator.
template <
    typename T_left,
    typename T_right,
    typename T_bind = bind_type<T_left, T_right>,
    typename T_eval_left = eval_type<T_left>,
    typename T_eval_right = eval_type<T_right>,
    typename T_return = eq_type<T_eval_left, T_eval_right>,
    typename T_token = operator_eq_t,
    typename T_type_constraint = typename std::enable_if<
        is_expression<T_left>::value || is_expression<T_right>::value>::type>
binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>
operator==(const T_left& left, const T_right& right)
{
    return binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>(
        expression_bind<T_bind>(left), expression_bind<T_bind>(right));
}

// != operator.
template <
    typename T_left,
    typename T_right,
    typename T_bind = bind_type<T_left, T_right>,
    typename T_eval_left = eval_type<T_left>,
    typename T_eval_right = eval_type<T_right>,
    typename T_return = ne_type<T_eval_left, T_eval_right>,
    typename T_token = operator_ne_t,
    typename T_type_constraint = typename std::enable_if<
        is_expression<T_left>::value || is_expression<T_right>::value>::type>
binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>
operator!=(const T_left& left, const T_right& right)
{
    return binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>(
        expression_bind<T_bind>(left), expression_bind<T_bind>(right));
}

// > operator.
template <
    typename T_left,
    typename T_right,
    typename T_bind = bind_type<T_left, T_right>,
    typename T_eval_left = eval_type<T_left>,
    typename T_eval_right = eval_type<T_right>,
    typename T_return = gt_type<T_eval_left, T_eval_right>,
    typename T_token = operator_gt_t,
    typename T_type_constraint = typename std::enable_if<
        is_expression<T_left>::value || is_expression<T_right>::value>::type>
binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>
operator>(const T_left& left, const T_right& right)
{
    return binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>(
        expression_bind<T_bind>(left), expression_bind<T_bind>(right));
}

// >= operator.
template <
    typename T_left,
    typename T_right,
    typename T_bind = bind_type<T_left, T_right>,
    typename T_eval_left = eval_type<T_left>,
    typename T_eval_right = eval_type<T_right>,
    typename T_return = ge_type<T_eval_left, T_eval_right>,
    typename T_token = operator_ge_t,
    typename T_type_constraint = typename std::enable_if<
        is_expression<T_left>::value || is_expression<T_right>::value>::type>
binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>
operator>=(const T_left& left, const T_right& right)
{
    return binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>(
        expression_bind<T_bind>(left), expression_bind<T_bind>(right));
}

// < operator.
template <
    typename T_left,
    typename T_right,
    typename T_bind = bind_type<T_left, T_right>,
    typename T_eval_left = eval_type<T_left>,
    typename T_eval_right = eval_type<T_right>,
    typename T_return = lt_type<T_eval_left, T_eval_right>,
    typename T_token = operator_lt_t,
    typename T_type_constraint = typename std::enable_if<
        is_expression<T_left>::value || is_expression<T_right>::value>::type>
binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>
operator<(const T_left& left, const T_right& right)
{
    return binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>(
        expression_bind<T_bind>(left), expression_bind<T_bind>(right));
}

// <= operator.
template <
    typename T_left,
    typename T_right,
    typename T_bind = bind_type<T_left, T_right>,
    typename T_eval_left = eval_type<T_left>,
    typename T_eval_right = eval_type<T_right>,
    typename T_return = le_type<T_eval_left, T_eval_right>,
    typename T_token = operator_le_t,
    typename T_type_constraint = typename std::enable_if<
        is_expression<T_left>::value || is_expression<T_right>::value>::type>
binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>
operator<=(const T_left& left, const T_right& right)
{
    return binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>(
        expression_bind<T_bind>(left), expression_bind<T_bind>(right));
}

// && operator.
template <
    typename T_left,
    typename T_right,
    typename T_bind = bind_type<T_left, T_right>,
    typename T_eval_left = eval_type<T_left>,
    typename T_eval_right = eval_type<T_right>,
    typename T_return = and_type<T_eval_left, T_eval_right>,
    typename T_token = operator_and_t,
    typename T_type_constraint = typename std::enable_if<
        is_expression<T_left>::value || is_expression<T_right>::value>::type>
binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>
operator&&(const T_left& left, const T_right& right)
{
    return binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>(
        expression_bind<T_bind>(left), expression_bind<T_bind>(right));
}

// || operator.
template <
    typename T_left,
    typename T_right,
    typename T_bind = bind_type<T_left, T_right>,
    typename T_eval_left = eval_type<T_left>,
    typename T_eval_right = eval_type<T_right>,
    typename T_return = or_type<T_eval_left, T_eval_right>,
    typename T_token = operator_or_t,
    typename T_type_constraint = typename std::enable_if<
        is_expression<T_left>::value || is_expression<T_right>::value>::type>
binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>
operator||(const T_left& left, const T_right& right)
{
    return binary_expression_t<T_bind, T_return, T_eval_left, T_eval_right, T_token>(
        expression_bind<T_bind>(left), expression_bind<T_bind>(right));
}

// ! operator.
template <
    typename T_operand,
    typename T_bind = bind_type<T_operand>,
    typename T_eval_operand = eval_type<T_operand>,
    typename T_return = not_type<T_eval_operand>,
    typename T_token = operator_not_t,
    typename T_type_constraint
    = typename std::enable_if<is_expression<T_operand>::value>::type>
unary_expression_t<T_bind, T_return, T_eval_operand, T_token>
operator!(
    const T_operand& operand)
{
    return unary_expression_t<T_bind, T_return, T_eval_operand, T_token>(
        expression_bind<T_bind>(operand));
}

} // namespace expressions
} // namespace gaia
