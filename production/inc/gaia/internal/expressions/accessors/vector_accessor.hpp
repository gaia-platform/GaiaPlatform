////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia/internal/expressions/accessors/member_accessor.hpp"
#include "gaia/internal/expressions/accessors/value_accessor.hpp"
#include "gaia/internal/expressions/transform_expression.hpp"

namespace gaia
{
namespace expressions
{

/**
 * This class represents an accessor to an array type in DAC.
 *
 * @tparam T_bind - context this expression binds to.
 * @tparam T_element - type of the elements of the vector.
 * @tparam T_return - the array type.
 */
template <typename T_bind, typename T_element, typename T_return>
class vector_accessor_t : public member_accessor_t<T_bind, T_return>
{
public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    vector_accessor_t(member_accessor_ptr_t<T_bind, T_return> accessor)
        : member_accessor_t<T_bind, T_return>(accessor)
    {
    }
    virtual ~vector_accessor_t() = default;

    subexpression_t<T_bind, T_return> create_subexpression() const override;

    transform_expression_t<T_bind, T_element> operator[](size_t) const;
    transform_expression_t<T_bind, T_element> operator[](const expression_t<T_bind, size_t>&) const;

    // Additional accessor operations taken directly from dac_array.
    transform_expression_t<T_bind, size_t> size() const;
};

#include "vector_accessor.inc"

} // namespace expressions
} // namespace gaia
