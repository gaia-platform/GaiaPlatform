/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>

#include "gaia/exceptions.hpp"

// QUESTION 1: should this be a common API instead of direct_access? If we want to use
//             optional outside
// QUESTION 2: what name should this class has? optional will collide with std::optional
//             we could use gaia_optional or dac_optional. If we add specific types to it,
//             such as `dac_optional_int`, the name can be become quite long.

namespace gaia
{
namespace direct_access
{

template <typename T_value>
struct optional_storage_t
{
    union
    {
        char dummy;
        T_value val;
    };
    bool m_has_value{false};

    optional_storage_t();
    explicit optional_storage_t(T_value value);
    ~optional_storage_t() = default;
};

template <typename T_value>
class optional
{
public:
    optional();

    // NOLINTNEXTLINE(google-explicit-constructor)
    optional(const T_value& t);

    ~optional() = default;

    void reset();
    bool has_value() const;

    T_value& value() &;
    const T_value& value() const&;

    T_value& operator*() &;
    const T_value& operator*() const&;

    template <typename T_default>
    T_value value_or(T_default&& default_value) const&;

private:
    optional_storage_t<T_value> m_storage;
};

#include "dac_optional.inc"

} // namespace direct_access
} // namespace gaia
