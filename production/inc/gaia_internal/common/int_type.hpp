/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup Common
 * @{
 */

/**
 * @brief This class is meant to help define type-safe custom integer-based types.
 * It should provide a type-safe alternative to using typedef.
 *
 * @tparam T The integer type used by this class.
 */
template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
class int_type_t
{
public:
    constexpr int_type_t()
    {
        m_value = 0;
    }

    constexpr int_type_t(T value)
    {
        m_value = value;
    }

    // This conversion operator will enable many direct operations with integers.
    operator T() const
    {
        return m_value;
    }

    // For explicit retrieval of contained value.
    T value() const
    {
        return m_value;
    }

    // For additional safety (so as to prevent the mixing of apples and oranges),
    // all these operators will only allow operations with identical types.
    int_type_t& operator+=(const int_type_t& other)
    {
        this->m_value += other.m_value;
        return *this;
    }

    int_type_t& operator-=(const int_type_t& other)
    {
        this->m_value -= other.m_value;
        return *this;
    }

    int_type_t& operator*=(const int_type_t& other)
    {
        this->m_value *= other.m_value;
        return *this;
    }

    int_type_t& operator/=(const int_type_t& other)
    {
        this->m_value /= other.m_value;
        return *this;
    }

    int_type_t& operator%=(const int_type_t& other)
    {
        this->m_value %= other.m_value;
        return *this;
    }

protected:
    T m_value;
};

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
