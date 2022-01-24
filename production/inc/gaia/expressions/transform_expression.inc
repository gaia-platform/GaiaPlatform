/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T_bind, typename T_return>
T_return transform_expression_t<T_bind, T_return>::operator()(const T_bind& bind) const
{
    return m_transform_fn(bind);
}

template <typename T_bind, typename T_return>
subexpression_t<T_bind, T_return> transform_expression_t<T_bind, T_return>::create_subexpression() const
{
    return std::make_shared<transform_expression_t<T_bind, T_return>>(*this);
}
