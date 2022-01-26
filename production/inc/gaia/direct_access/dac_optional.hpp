/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>

#include "gaia/exceptions.hpp"

// QUESTION 1: Should this be a common API instead of direct_access? If we want to use
//             optional outside DAC classes we may consider this.
// QUESTION 2: What name should this class have? optional will collide with std::optional
//             we could use gaia_optional or dac_optional. If we add specific types to it,
//             such as `dac_optional_int`, the name can be become quite long.
// QUESTION 3: Should we limit the feature of this class to scalar types only (since we don't
//             need it for anything else for now)
// QUESTION 4: Should we replace nullable_string with this?

namespace gaia
{
namespace direct_access
{

// TODO Add documentation. Want to have consensus on the
//  feature before spending time documenting.

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

    T_value& value() &;
    const T_value& value() const&;

    template <typename T_default>
    T_value value_or(T_default&& default_value) const&;

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

#include "dac_optional.inc"

} // namespace direct_access
} // namespace gaia
