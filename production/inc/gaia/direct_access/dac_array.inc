////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

//
// Implementation for dac_vector_const_iterator_t.
//

template <typename T_type>
dac_vector_const_iterator_t<T_type>::dac_vector_const_iterator_t(const dac_vector_t<T_type>& dac_vector, uint32_t index)
    : m_iterator_data(dac_vector.data()), m_index(index)
{
}

template <typename T_type>
bool dac_vector_const_iterator_t<T_type>::operator==(const dac_vector_const_iterator_t& rhs) const
{
    return m_index == rhs.m_index;
}

template <typename T_type>
bool dac_vector_const_iterator_t<T_type>::operator!=(const dac_vector_const_iterator_t& rhs) const
{
    return !(*this == rhs);
}

template <typename T_type>
typename dac_vector_const_iterator_t<T_type>::reference dac_vector_const_iterator_t<T_type>::operator*() const
{
    return *(m_iterator_data + m_index);
}

template <typename T_type>
typename dac_vector_const_iterator_t<T_type>::pointer dac_vector_const_iterator_t<T_type>::operator->() const
{
    return m_iterator_data + m_index;
}

template <typename T_type>
dac_vector_const_iterator_t<T_type>& dac_vector_const_iterator_t<T_type>::operator++()
{
    ++m_index;
    return *this;
}

template <typename T_type>
dac_vector_const_iterator_t<T_type> dac_vector_const_iterator_t<T_type>::operator++(int)
{
    dac_vector_const_iterator_t<T_type> temp = *this;
    ++*this;
    return temp;
}

template <typename T_type>
dac_vector_const_iterator_t<T_type>& dac_vector_const_iterator_t<T_type>::operator--()
{
    --m_index;
    return *this;
}

template <typename T_type>
dac_vector_const_iterator_t<T_type> dac_vector_const_iterator_t<T_type>::operator--(int)
{
    dac_vector_const_iterator_t<T_type> temp = *this;
    --*this;
    return temp;
}

template <typename T_type>
dac_vector_const_iterator_t<T_type> dac_vector_const_iterator_t<T_type>::operator+(
    dac_vector_const_iterator_t<T_type>::difference_type rhs) const
{
    dac_vector_const_iterator_t<T_type> iter_copy = this;
    return iter_copy += rhs;
}

template <typename T_type>
dac_vector_const_iterator_t<T_type> operator+(
    typename dac_vector_const_iterator_t<T_type>::difference_type lhs,
    const dac_vector_const_iterator_t<T_type>& rhs)
{
    return rhs + lhs;
}

template <typename T_type>
dac_vector_const_iterator_t<T_type> dac_vector_const_iterator_t<T_type>::operator-(
    dac_vector_const_iterator_t<T_type>::difference_type rhs) const
{
    dac_vector_const_iterator_t<T_type> iter_copy = this;
    return iter_copy -= rhs;
}

template <typename T_type>
typename dac_vector_const_iterator_t<T_type>::difference_type dac_vector_const_iterator_t<T_type>::operator-(
    const dac_vector_const_iterator_t<T_type>& rhs) const
{
    return m_index - rhs.m_index;
}

template <typename T_type>
dac_vector_const_iterator_t<T_type>& dac_vector_const_iterator_t<T_type>::operator+=(
    dac_vector_const_iterator_t<T_type>::difference_type rhs)
{
    m_index += rhs;
    return *this;
}

template <typename T_type>
dac_vector_const_iterator_t<T_type>& dac_vector_const_iterator_t<T_type>::operator-=(
    dac_vector_const_iterator_t<T_type>::difference_type rhs)
{
    m_index -= rhs;
    return *this;
}

template <typename T_type>
typename dac_vector_const_iterator_t<T_type>::reference dac_vector_const_iterator_t<T_type>::operator[](
    dac_vector_const_iterator_t<T_type>::difference_type rhs) const
{
    return *(*this + rhs);
}

template <typename T_type>
bool dac_vector_const_iterator_t<T_type>::operator<(const dac_vector_const_iterator_t<T_type>& rhs) const
{
    return m_index < rhs.m_index;
}

template <typename T_type>
bool dac_vector_const_iterator_t<T_type>::operator>(const dac_vector_const_iterator_t<T_type>& rhs) const
{
    return rhs < *this;
}

template <typename T_type>
bool dac_vector_const_iterator_t<T_type>::operator<=(const dac_vector_const_iterator_t<T_type>& rhs) const
{
    return !(*this > rhs);
}

template <typename T_type>
bool dac_vector_const_iterator_t<T_type>::operator>=(const dac_vector_const_iterator_t<T_type>& rhs) const
{
    return !(*this < rhs);
}

//
// Implementation for dac_vector_const_reverse_iterator_t.
//

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type>::dac_vector_const_reverse_iterator_t(const dac_vector_t<T_type>& dac_vector, uint32_t index)
    : m_iterator_data(dac_vector.data()), m_index(index)
{
}

template <typename T_type>
bool dac_vector_const_reverse_iterator_t<T_type>::operator==(const dac_vector_const_reverse_iterator_t& rhs) const
{
    return m_index == rhs.m_index;
}

template <typename T_type>
bool dac_vector_const_reverse_iterator_t<T_type>::operator!=(const dac_vector_const_reverse_iterator_t& rhs) const
{
    return !(*this == rhs);
}

template <typename T_type>
typename dac_vector_const_reverse_iterator_t<T_type>::reference dac_vector_const_reverse_iterator_t<T_type>::operator*() const
{
    return *(m_iterator_data + m_index - 1);
}

template <typename T_type>
typename dac_vector_const_reverse_iterator_t<T_type>::pointer dac_vector_const_reverse_iterator_t<T_type>::operator->() const
{
    return m_iterator_data + m_index - 1;
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type>& dac_vector_const_reverse_iterator_t<T_type>::operator++()
{
    --m_index;
    return *this;
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type> dac_vector_const_reverse_iterator_t<T_type>::operator++(int)
{
    dac_vector_const_reverse_iterator_t<T_type> temp = *this;
    ++*this;
    return temp;
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type>& dac_vector_const_reverse_iterator_t<T_type>::operator--()
{
    ++m_index;
    return *this;
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type> dac_vector_const_reverse_iterator_t<T_type>::operator--(int)
{
    dac_vector_const_reverse_iterator_t<T_type> temp = *this;
    --*this;
    return temp;
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type> dac_vector_const_reverse_iterator_t<T_type>::operator+(
    dac_vector_const_reverse_iterator_t<T_type>::difference_type rhs) const
{
    dac_vector_const_reverse_iterator_t<T_type> iter_copy = this;
    return iter_copy += rhs;
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type> operator+(
    typename dac_vector_const_reverse_iterator_t<T_type>::difference_type lhs,
    const dac_vector_const_reverse_iterator_t<T_type>& rhs)
{
    return rhs + lhs;
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type> dac_vector_const_reverse_iterator_t<T_type>::operator-(
    dac_vector_const_reverse_iterator_t<T_type>::difference_type rhs) const
{
    dac_vector_const_reverse_iterator_t<T_type> iter_copy = this;
    return iter_copy -= rhs;
}

template <typename T_type>
typename dac_vector_const_reverse_iterator_t<T_type>::difference_type dac_vector_const_reverse_iterator_t<T_type>::operator-(
    const dac_vector_const_reverse_iterator_t<T_type>& rhs) const
{
    return m_index + rhs.m_index;
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type>& dac_vector_const_reverse_iterator_t<T_type>::operator+=(
    dac_vector_const_reverse_iterator_t<T_type>::difference_type rhs)
{
    m_index -= rhs;
    return *this;
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type>& dac_vector_const_reverse_iterator_t<T_type>::operator-=(
    dac_vector_const_reverse_iterator_t<T_type>::difference_type rhs)
{
    m_index += rhs;
    return *this;
}

template <typename T_type>
typename dac_vector_const_reverse_iterator_t<T_type>::reference dac_vector_const_reverse_iterator_t<T_type>::operator[](
    dac_vector_const_reverse_iterator_t<T_type>::difference_type rhs) const
{
    return *(*this - rhs);
}

template <typename T_type>
bool dac_vector_const_reverse_iterator_t<T_type>::operator<(const dac_vector_const_reverse_iterator_t<T_type>& rhs) const
{
    return m_index > rhs.m_index;
}

template <typename T_type>
bool dac_vector_const_reverse_iterator_t<T_type>::operator>(const dac_vector_const_reverse_iterator_t<T_type>& rhs) const
{
    return rhs < *this;
}

template <typename T_type>
bool dac_vector_const_reverse_iterator_t<T_type>::operator<=(const dac_vector_const_reverse_iterator_t<T_type>& rhs) const
{
    return !(*this > rhs);
}

template <typename T_type>
bool dac_vector_const_reverse_iterator_t<T_type>::operator>=(const dac_vector_const_reverse_iterator_t<T_type>& rhs) const
{
    return !(*this < rhs);
}

//
// Implementation for dac_vector_t.
//

template <typename T_type>
const T_type* dac_vector_t<T_type>::data() const
{
    return m_vector->data();
}

template <typename T_type>
uint32_t dac_vector_t<T_type>::size() const
{
    m_vector->begin();
    return m_vector->size();
}

/**
 * Normally `operator[]` should return a reference or const reference
 * to the array element. Given we only support arrays of basic types and the
 * vector class is always read-only, it should be safe to return the T_type
 * value directly here without any performance or functionality loss.
 */
template <typename T_type>
T_type dac_vector_t<T_type>::operator[](uint32_t i) const
{
    return (*m_vector)[i];
}

template <typename T_type>
dac_vector_const_iterator_t<T_type> dac_vector_t<T_type>::begin() const
{
    return dac_vector_const_iterator_t<T_type>(*this, 0);
}

template <typename T_type>
dac_vector_const_iterator_t<T_type> dac_vector_t<T_type>::end() const
{
    return dac_vector_const_iterator_t<T_type>(*this, size());
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type> dac_vector_t<T_type>::rbegin() const
{
    return dac_vector_const_reverse_iterator_t<T_type>(*this, size());
}

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type> dac_vector_t<T_type>::rend() const
{
    return dac_vector_const_reverse_iterator_t<T_type>(*this, 0);
}

template <typename T_type>
std::vector<T_type> dac_vector_t<T_type>::to_vector() const
{
    return std::vector<T_type>(data(), data() + size());
}

template <typename T_type>
dac_vector_t<T_type>::dac_vector_t(const flatbuffers::Vector<T_type>* vector_ptr)
    : m_vector(vector_ptr)
{
    static_assert(std::is_arithmetic<T_type>::value, "dac_vector_t only supports basic types!");
};
