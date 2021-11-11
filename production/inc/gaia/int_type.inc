/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T>
constexpr int_type_t<T>::int_type_t()
{
    m_value = 0;
}

template <typename T>
constexpr int_type_t<T>::int_type_t(T value)
{
    m_value = value;
}

// This conversion operator will enable many direct operations with integers.
template <typename T>
int_type_t<T>::operator T() const
{
    return m_value;
}

// For explicit retrieval of contained value.
template <typename T>
constexpr T int_type_t<T>::value() const
{
    return m_value;
}

// For direct updating of contained value.
template <typename T>
T* int_type_t<T>::value_ptr()
{
    return &m_value;
}

// For additional safety (so as to prevent the mixing of apples and oranges),
// all these operators will only allow operations with identical types.
template <typename T>
int_type_t<T>& int_type_t<T>::operator+=(const int_type_t<T>& other)
{
    this->m_value += other.m_value;
    return *this;
}

template <typename T>
int_type_t<T>& int_type_t<T>::operator-=(const int_type_t<T>& other)
{
    this->m_value -= other.m_value;
    return *this;
}

template <typename T>
int_type_t<T>& int_type_t<T>::operator*=(const int_type_t<T>& other)
{
    this->m_value *= other.m_value;
    return *this;
}

template <typename T>
int_type_t<T>& int_type_t<T>::operator/=(const int_type_t<T>& other)
{
    this->m_value /= other.m_value;
    return *this;
}

template <typename T>
int_type_t<T>& int_type_t<T>::operator%=(const int_type_t<T>& other)
{
    this->m_value %= other.m_value;
    return *this;
}

template <typename T>
int_type_t<T>& int_type_t<T>::operator++()
{
    this->m_value += 1;
    return *this;
}

template <typename T>
int_type_t<T> int_type_t<T>::operator++(int)
{
    int_type_t pre_value = *this;
    this->m_value += 1;
    return pre_value;
}

template <typename T>
int_type_t<T>& int_type_t<T>::operator--()
{
    this->m_value -= 1;
    return *this;
}

template <typename T>
int_type_t<T> int_type_t<T>::operator--(int)
{
    int_type_t pre_value = *this;
    this->m_value -= 1;
    return pre_value;
}
