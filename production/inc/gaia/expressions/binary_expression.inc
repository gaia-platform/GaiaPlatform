/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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
