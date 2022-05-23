////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <type_traits>

#include "gaia/internal/expressions/accessors/member_accessor.hpp"
#include "gaia/internal/expressions/transform_expression.hpp"

// Specialization of member_accessor_t for container types.
namespace gaia
{
namespace expressions
{

/**
 * This class provides a specialized accessor on fields of container type.
 *
 * @tparam T_bind - the parent class with this container.
 * @tparam T_value - the type of objects inside this container.
 * @tparam T_return - the container type.
 */
template <typename T_bind, typename T_value, typename T_return>
class container_accessor_t : public member_accessor_t<T_bind, T_return>
{
public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    container_accessor_t(member_accessor_ptr_t<T_bind, T_return> accessor)
        : member_accessor_t<T_bind, T_return>(accessor)
    {
    }
    virtual ~container_accessor_t() = default;

    subexpression_t<T_bind, T_return> create_subexpression() const override;

    // Additional APIs on top of dac_container types.
    transform_expression_t<T_bind, bool> contains(const expression_t<T_value, bool>& predicate);
    transform_expression_t<T_bind, bool> contains(const T_value& value);
    transform_expression_t<T_bind, bool> empty();
    transform_expression_t<T_bind, int64_t> count();
};

#include "container_accessor.inc"

} // namespace expressions
} // namespace gaia
