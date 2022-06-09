////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

template <typename T_bind, typename T_return>
T_return member_accessor_t<T_bind, T_return>::operator()(const T_bind& bind) const
{
    return std::cref(m_member_accessor)(bind);
}

template <typename T_bind, typename T_return>
subexpression_t<T_bind, T_return> member_accessor_t<T_bind, T_return>::create_subexpression() const
{
    return std::make_shared<member_accessor_t<T_bind, T_return>>(*this);
}
