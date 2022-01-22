/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>

#include "gaia/expressions/expression_ast.hpp"
#include "gaia/expressions/expression_traits.hpp"

namespace gaia
{
namespace expressions
{

// This contains logic to bind objects to a specific expression context.
// Unbound objects are cast to the expression equivalent.

template <
    typename T_bind,
    typename T_class,
    typename = typename std::enable_if<is_value<T_class>::value>::type>
constexpr value_accessor_t<T_bind, T_class, eval_type<T_class>> expression_bind(const T_class& value)
{
    return value_accessor_t<T_bind, T_class, eval_type<T_class>>(value);
}

// IDENTITY: all expressions are already bound!
// Check if the expression binds to a different context.. return a compiler error if so.
template <typename T_bind, typename T_expr_bind, typename T_return>
constexpr const expression_t<T_bind, T_return>&
expression_bind(
    const expression_t<T_expr_bind, T_return>& expression)
{
    static_assert(std::is_same<T_bind, T_expr_bind>::value, "Cannot bind expression to this context.");
    return expression;
}

} // namespace expressions
} // namespace gaia
