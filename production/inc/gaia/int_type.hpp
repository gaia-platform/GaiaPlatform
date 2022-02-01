/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <limits>
#include <type_traits>

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup common
 * @{
 */

/**
 * @brief This class defines a base class for type-safe custom integer-based types.
 * It provides a type-safe alternative to using typedef.
 *
 * @tparam T_type The integer type used by this class.
 * @tparam default_invalid_value The default invalid value to be used with this type.
 * @tparam T_type_constraint A constraint on the types accepted for T_type.
 */
template <typename T_type, T_type default_invalid_value, typename T_type_constraint = std::enable_if_t<std::is_integral<T_type>::value>>
class int_type_t
{
public:
    using value_type = T_type;

    static constexpr T_type c_default_invalid_value = default_invalid_value;

    constexpr int_type_t()
        : m_value(default_invalid_value)
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr int_type_t(T_type value)
        : m_value(value)
    {
    }

    // This conversion operator will enable many direct operations with integers.
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator T_type() const;

    // Return false if the contained value is c_default_invalid_value and true otherwise.
    explicit operator bool() const;

    // For explicit retrieval of contained value.
    constexpr T_type value() const
    {
        return m_value;
    }

    // For direct updating of contained value.
    T_type& value_ref();

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
    T_type m_value;
};

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
