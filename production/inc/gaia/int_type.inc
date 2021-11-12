/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T>
constexpr int_type_t<T>::int_type_t()
    : m_value(0)
{
}

template <typename T>
constexpr int_type_t<T>::int_type_t(T value)
    : m_value(value)
{
}

// For explicit retrieval of contained value.
template <typename T>
constexpr T int_type_t<T>::value() const
{
    return m_value;
}
