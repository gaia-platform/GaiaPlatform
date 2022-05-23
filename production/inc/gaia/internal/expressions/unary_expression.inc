////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

template <typename T_bind, typename T_return, typename T_operand, typename T_token>
unary_expression_t<T_bind, T_return, T_operand, T_token>::unary_expression_t(
    const expression_t<T_bind, T_operand>& operand)
    : m_function(
        [](const T_bind& bind, const subexpression_t<T_bind, T_operand>& operand_expression) {
            return evaluate_operator((*operand_expression)(bind), T_token{});
        })
    , m_operand(operand.create_subexpression())
{
}

template <typename T_bind, typename T_return, typename T_operand, typename T_token>
T_return unary_expression_t<T_bind, T_return, T_operand, T_token>::operator()(const T_bind& bind) const
{
    return m_function(bind, m_operand);
}

template <typename T_bind, typename T_return, typename T_operand, typename T_token>
subexpression_t<T_bind, T_return> unary_expression_t<T_bind, T_return, T_operand, T_token>::create_subexpression() const
{
    return std::make_shared<unary_expression_t<T_bind, T_return, T_operand, T_token>>(*this);
}
