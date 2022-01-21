template <typename T_bind, typename T_return>
value_accessor_t<T_bind, T_return>::value_accessor_t(const T_return& value)
    : m_value(value)
{
}

template <typename T_bind, typename T_return>
T_return value_accessor_t<T_bind, T_return>::operator()(const T_bind&) const
{
    return m_value;
}

template <typename T_bind, typename T_return>
subexpression_t<T_bind, T_return> value_accessor_t<T_bind, T_return>::create_subexpression() const
{
    return std::make_shared<value_accessor_t<T_bind, T_return>>(*this);
}
