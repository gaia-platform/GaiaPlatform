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
 * Unary expression - expression consisting of one operator + one operand.
 * @tparam T_bind - context this expression binds to.
 * @tparam T_return - type this expression evaluates to.
 * @tparam T_operand - the type the operand evaluates to.
 *
 * unary_expression_fn_t - function evaluating a the subexpression then performs the operation.
 *
 */

template <typename T_bind, typename T_return>
using unary_expression_fn_t = std::function<T_return(const T_bind&, const subexpression_t<T_bind, T_return>&)>;

template <typename T_bind, typename T_return, typename T_operand, typename T_token>
class unary_expression_t : public expression_t<T_bind, T_return>
{
public:
    explicit unary_expression_t(
        const expression_t<T_bind, T_operand>& operand);
    virtual ~unary_expression_t() = default;

    T_return operator()(const T_bind& bind) const override;
    subexpression_t<T_bind, T_return> create_subexpression() const override;

private:
    unary_expression_fn_t<T_bind, T_return> m_function;
    subexpression_t<T_bind, T_operand> m_operand;
};

#include "unary_expression.inc"

} // namespace expressions
} // namespace gaia
