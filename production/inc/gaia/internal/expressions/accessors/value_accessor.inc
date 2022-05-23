////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

template <typename T_bind, typename T_boxed, typename T_return>
value_accessor_t<T_bind, T_boxed, T_return>::value_accessor_t(const T_boxed& value)
    : m_value(value)
{
}

template <typename T_bind, typename T_boxed, typename T_return>
T_return value_accessor_t<T_bind, T_boxed, T_return>::operator()(const T_bind&) const
{
    return m_value;
}

template <typename T_bind, typename T_boxed, typename T_return>
subexpression_t<T_bind, T_return> value_accessor_t<T_bind, T_boxed, T_return>::create_subexpression() const
{
    return std::make_shared<value_accessor_t<T_bind, T_boxed, T_return>>(*this);
}
