/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/expressions/expression_base.hpp"
#include "gaia/expressions/expression_traits.hpp"
#include "gaia/expressions/operators.hpp"

namespace gaia
{
namespace expressions
{

/**
 * Binary expression - this expression denotes an expression with a left and right operand.
 * @tparam T_bind - context this expression binds to.
 * @tparam T_return - the type this expression evaluates to.
 * @tparam T_left - the evaluated type of the left operand.
 * @tparam T_right - the evaluated type of the right operand.
 *
 * binary_expression_fn_t - expression evaluating left operand, right operand and then applying the operation.
 */

template <typename T_bind, typename T_return, typename T_left, typename T_right>
using binary_expression_fn_t = std::function<T_return(
    T_bind,
    const subexpression_t<T_bind, T_left>&,
    const subexpression_t<T_bind, T_right>&)>;

template <typename T_bind, typename T_return, typename T_left, typename T_right, typename T_token>
class binary_expression_t : public expression_t<T_bind, T_return>
{
public:
    binary_expression_t(
        const expression_t<T_bind, T_left>& left_operand,
        const expression_t<T_bind, T_right>& right_operand);
    virtual ~binary_expression_t() = default;

    T_return operator()(const T_bind& bind) const override;
    subexpression_t<T_bind, T_return> create_subexpression() const override;

private:
    binary_expression_fn_t<T_bind, T_return, T_left, T_right> m_function;

    subexpression_t<T_bind, T_left> m_left_operand;
    subexpression_t<T_bind, T_right> m_right_operand;
};

#include "binary_expression.inc"

} // namespace expressions
} // namespace gaia
