/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T_bind, typename T_return, typename T_operand, operator_token_t e_token>
T_return unary_expression_t<T_bind, T_return, T_operand, e_token>::operator()(const T_bind& bind) const
{
    return m_function(bind, m_operand);
}

template <typename T_bind, typename T_return, typename T_operand, operator_token_t e_token>
subexpression_t<T_bind, T_return> unary_expression_t<T_bind, T_return, T_operand, e_token>::create_subexpression() const
{
    return std::make_shared<unary_expression_t<T_bind, T_return, T_operand, e_token>>(*this);
}
