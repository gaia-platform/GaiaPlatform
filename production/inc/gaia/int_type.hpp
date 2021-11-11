/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>

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
template <typename T>
class int_type_t
{
public:
    constexpr int_type_t();
    constexpr int_type_t(T value);

    // This conversion operator will enable many direct operations with integers.
    operator T() const;

    // For explicit retrieval of contained value.
    constexpr T value() const;

    // For direct updating of contained value.
    T* value_ptr();

    // For additional safety (so as to prevent the mixing of apples and oranges),
    // all these operators will only allow operations with identical types.
    int_type_t& operator+=(const int_type_t& other);
    int_type_t& operator-=(const int_type_t& other);
    int_type_t& operator*=(const int_type_t& other);
    int_type_t& operator/=(const int_type_t& other);
    int_type_t& operator%=(const int_type_t& other);

    int_type_t& operator++();
    int_type_t operator++(int);

    int_type_t& operator--();
    int_type_t operator--(int);

protected:
    T m_value;
};

#include "int_type.inc"

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
