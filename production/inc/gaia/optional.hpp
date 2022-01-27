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
 * Encapsulate a value of type T_vale or the absence of it.
 *
 * This class resembles the C++17 std::optional class.
 * The Gaia API need to be compatible with C++11, this is the main
 * reason to introduce a lightweight custom implementation.
 * We try to be similar to std::optional to make it familiar
 * to C++17 users. Not all API have been implemented and some
 * custom API could be added. Basic functionality std::optional
 * functionality is retained.
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
