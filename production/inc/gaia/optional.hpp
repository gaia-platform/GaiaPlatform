/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>

#include "gaia/exceptions.hpp"

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
 * nullopt_t is a tag type that denotes an optional_t initilized in
 * an empty state.
 */
struct nullopt_t
{
    // The integer in the constructor ensures that {} can’t be mistaken for nullopt_t{};
    constexpr explicit nullopt_t(uint8_t){};
};

/// Constant of type std::nullopt_t that is used to indicate optional type with uninitialized state.
// NOLINTNEXTLINE(readability-identifier-naming)
constexpr nullopt_t nullopt{42};

/**
 * Encapsulate a value of type T_value or the absence of it.
 *
 * The Gaia API is designed to be compatible with C++11.
 * C++ 11 does not provide a mechanism to handle optional values.
 * This class is a lightweight custom implementation that
 * resembles the C++17 std::optional class and should be familiar
 * to developers using C++ 17 and higher.
 * This class does not implement the entire std::optional API set
 * and we may consider to add some custom API.
 *
 * The design of this class is inspired by:
 * - http://www.club.cc.cmu.edu/~ajo/disseminate/2017-02-15-Optional-From-Scratch.pdf
 * - https://github.com/akrzemi1/Optional/blob/master/optional.hpp
 *
 * @tparam T_value The type of the value optionally stored by this class.
 */
template <typename T_value>
class optional_t
{
public:
    optional_t();

    // NOLINTNEXTLINE(google-explicit-constructor)
    optional_t(const T_value& t);
    // NOLINTNEXTLINE(google-explicit-constructor)
    optional_t(nullopt_t);

    optional_t& operator=(nullopt_t) noexcept;

    ~optional_t() = default;

    void reset();
    bool has_value() const;
    explicit operator bool() const;

    T_value& value();
    const T_value& value() const;

    T_value& operator*();
    const T_value& operator*() const;

private:
    struct optional_storage_t
    {
        union
        {
            char empty;
            T_value value;
        };

        bool has_value{false};

        optional_storage_t();
        explicit optional_storage_t(T_value value);
        ~optional_storage_t() = default;
    };

private:
    optional_storage_t m_storage;
};

#include "optional.inc"

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
