/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <type_traits>

#include "expression_base.hpp"
#include "expression_traits.hpp"

namespace gaia
{
namespace expressions
{

/**
 * This is an expression boxing a native type.
 * As such, it is only enabled for non-expression types.
 * @tparam T_bind - context this expression binds to.
 * @tparam T_return - what this expression evaluates to.
 */
template <typename T_bind, typename T_return, typename = typename std::enable_if<!is_expression<T_return>::value>::type>
class value_accessor_t;

template <typename T_bind, typename T_return>
class value_accessor_t<T_bind, T_return> final : public expression_t<T_bind, T_return>
{
public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    value_accessor_t(const T_return& value);
    virtual ~value_accessor_t() = default;

    T_return operator()(const T_bind& bind) const override;
    subexpression_t<T_bind, T_return> create_subexpression() const override;

private:
    const T_return& m_value;
};

#include "value_accessor.inc"

} // namespace expressions
} // namespace gaia
