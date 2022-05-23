////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

template <typename T_bind, typename T_return, typename T_left, typename T_right, typename T_token>
binary_expression_t<T_bind, T_return, T_left, T_right, T_token>::binary_expression_t(
    const expression_t<T_bind, T_left>& left_operand,
    const expression_t<T_bind, T_right>& right_operand)
    : m_function([](
                     const T_bind& bind,
                     const subexpression_t<T_bind, T_left>& left_expression,
                     const subexpression_t<T_bind, T_right>& right_expression) {
        return evaluate_operator((*left_expression)(bind), (*right_expression)(bind), T_token{});
    })
    , m_left_operand(left_operand.create_subexpression())
    , m_right_operand(right_operand.create_subexpression())
{
}

template <typename T_bind, typename T_return, typename T_left, typename T_right, typename T_token>
T_return binary_expression_t<T_bind, T_return, T_left, T_right, T_token>::operator()(const T_bind& bind) const
{
    return m_function(bind, m_left_operand, m_right_operand);
}

template <typename T_bind, typename T_return, typename T_left, typename T_right, typename T_token>
subexpression_t<T_bind, T_return> binary_expression_t<T_bind, T_return, T_left, T_right, T_token>::create_subexpression() const
{
    return std::make_shared<binary_expression_t<T_bind, T_return, T_left, T_right, T_token>>(*this);
}
