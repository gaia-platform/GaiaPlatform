////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

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
