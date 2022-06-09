////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once
#include <type_traits>

#include "gaia/expressions/expression_base.hpp"
#include "gaia/expressions/expression_traits.hpp"

namespace gaia
{
namespace expressions
{

/**
 * This is an expression boxing a native type.
 * As such, it is only enabled for non-expression types.
 * @tparam T_bind - context this expression binds to.
 * @tparam T_boxed - type of the value of the to be boxed.
 * @tparam T_return - what this expression evaluates to.
 */
template <
    typename T_bind,
    typename T_boxed,
    typename T_return = eval_type<T_boxed>,
    typename = typename std::enable_if<is_value<T_boxed>::value>::type>
class value_accessor_t;

template <typename T_bind, typename T_boxed, typename T_return>
class value_accessor_t<T_bind, T_boxed, T_return> final : public expression_t<T_bind, T_return>
{
public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    value_accessor_t(const T_boxed& value);
    virtual ~value_accessor_t() = default;

    T_return operator()(const T_bind& bind) const override;
    subexpression_t<T_bind, T_return> create_subexpression() const override;

private:
    T_return m_value;
};

#include "value_accessor.inc"

} // namespace expressions
} // namespace gaia
