/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

//
// Implementation for dac_vector_iterator_t.
//
template <typename T_class>
dac_vector_iterator_t<T_class>::dac_vector_iterator_t()
    : m_iterator_data(nullptr), m_index(0)
{
}

template <typename T_class>
dac_vector_iterator_t<T_class>::dac_vector_iterator_t(const T_class* iterator_data, uint32_t index)
    : m_iterator_data(iterator_data), m_index(index)
{
}

template <typename T_class>
dac_vector_iterator_t<T_class>& dac_vector_iterator_t<T_class>::operator++()
{
    ++m_index;
    return *this;
}

template <typename T_class>
dac_vector_iterator_t<T_class> dac_vector_iterator_t<T_class>::operator++(int)
{
    dac_vector_iterator_t<T_class> temp = *this;
    ++*this;
    return temp;
}

template <typename T_class>
bool dac_vector_iterator_t<T_class>::operator==(const dac_vector_iterator_t& rhs) const
{
    return m_iterator_data == rhs.m_iterator_data && m_index == rhs.m_index;
}

template <typename T_class>
bool dac_vector_iterator_t<T_class>::operator!=(const dac_vector_iterator_t& rhs) const
{
    return m_iterator_data != rhs.m_iterator_data || m_index != rhs.m_index;
}

template <typename T_class>
const T_class& dac_vector_iterator_t<T_class>::operator*() const
{
    return *(m_iterator_data + m_index);
}

template <typename T_class>
const T_class* dac_vector_iterator_t<T_class>::operator->() const
{
    return m_iterator_data + m_index;
}

//
// Implementation for dac_vector_iterator_t.
//
template <typename T_type>
const T_type* dac_vector_t<T_type>::data() const
{
    return m_vector->data();
}

template <typename T_type>
uint32_t dac_vector_t<T_type>::size() const
{
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
dac_vector_iterator_t<T_type> dac_vector_t<T_type>::begin() const
{
    return dac_vector_iterator_t<T_type>(data(), 0);
}

template <typename T_type>
dac_vector_iterator_t<T_type> dac_vector_t<T_type>::end() const
{
    return dac_vector_iterator_t<T_type>(data(), size());
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
