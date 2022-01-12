/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "expression_base.hpp"
#include "expression_traits.hpp"

namespace gaia
{
namespace expressions
{

/*
 * Binary expression - this expression denotes an expression with a left and right operand.
 * T_bind - context this expression binds to.
 * T_return - the type this expression evaluates to.
 * T_left - the evaluated type of the left operand.
 * T_right - the evaluated type of the right operand.
 *
 * binary_expression_fn_t - expression evaluating left operand, right operand and then applying the operation.
 */

template <typename T_bind, typename T_return, typename T_left, typename T_right>
using binary_expression_fn_t = std::function<T_return(
    T_bind,
    const subexpression_t<T_bind, T_left>&,
    const subexpression_t<T_bind, T_right>&)>;

template <typename T_bind, typename T_return, typename T_left, typename T_right, operator_token_t e_token>
class binary_expression_t : public expression_t<T_bind, T_return>
{
public:
    binary_expression_t(
        const expression_t<T_bind, T_left>& left_operand,
        const expression_t<T_bind, T_right>& right_operand,
        binary_expression_fn_t<T_bind, T_return, T_left, T_right> function)
        : m_function(function), m_left_operand(left_operand.create_subexpression()), m_right_operand(right_operand.create_subexpression()){};
    virtual ~binary_expression_t() = default;

    T_return operator()(const T_bind&) const override;
    subexpression_t<T_bind, T_return> create_subexpression() const override;

private:
    binary_expression_fn_t<T_bind, T_return, T_left, T_right> m_function;

    subexpression_t<T_bind, T_left> m_left_operand;
    subexpression_t<T_bind, T_right> m_right_operand;
};

template <typename T_bind, typename T_return, typename T_left, typename T_right, operator_token_t e_token>
T_return binary_expression_t<T_bind, T_return, T_left, T_right, e_token>::operator()(const T_bind& bind) const
{
    return m_function(bind, m_left_operand, m_right_operand);
}

template <typename T_bind, typename T_return, typename T_left, typename T_right, operator_token_t e_token>
subexpression_t<T_bind, T_return> binary_expression_t<T_bind, T_return, T_left, T_right, e_token>::create_subexpression() const
{
    return std::make_shared<binary_expression_t<T_bind, T_return, T_left, T_right, e_token>>(*this);
}

} // namespace expressions
} // namespace gaia
