/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T_bind, typename T_class, typename T_return>
value_accessor_t<T_bind, T_class, T_return>::value_accessor_t(const T_class& value)
    : m_value(value)
{
}

template <typename T_bind, typename T_class, typename T_return>
T_return value_accessor_t<T_bind, T_class, T_return>::operator()(const T_bind&) const
{
    return m_value;
}

template <typename T_bind, typename T_class, typename T_return>
subexpression_t<T_bind, T_return> value_accessor_t<T_bind, T_class, T_return>::create_subexpression() const
{
    return std::make_shared<value_accessor_t<T_bind, T_class, T_return>>(*this);
}
