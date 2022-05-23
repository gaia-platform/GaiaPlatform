////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

template <typename T_bind, typename T_element, typename T_return>
transform_expression_t<T_bind, T_element>
vector_accessor_t<T_bind, T_element, T_return>::operator[](size_t array_access) const
{
    return transform_expression_t<T_bind, T_element>(
        [this, array_access](
            const T_bind& bind) {
            return (*this)(bind)[array_access];
        });
}

template <typename T_bind, typename T_element, typename T_return>
transform_expression_t<T_bind, T_element>
vector_accessor_t<T_bind, T_element, T_return>::operator[](const expression_t<T_bind, size_t>& array_access) const
{
    return transform_expression_t<T_bind, T_element>(
        [this, &array_access](
            const T_bind& bind) {
            return (*this)(bind)[std::cref(array_access)(bind)];
        });
}

template <typename T_bind, typename T_element, typename T_return>
transform_expression_t<T_bind, size_t>
vector_accessor_t<T_bind, T_element, T_return>::size() const
{
    return transform_expression_t<T_bind, size_t>(
        [this](
            const T_bind& bind) {
            return (*this)(bind).size();
        });
}

template <typename T_bind, typename T_element, typename T_return>
subexpression_t<T_bind, T_return> vector_accessor_t<T_bind, T_element, T_return>::create_subexpression() const
{
    return std::make_shared<vector_accessor_t<T_bind, T_element, T_return>>(*this);
}
