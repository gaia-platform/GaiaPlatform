////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <string>
#include <type_traits>

#include "gaia/expressions/expression_base.hpp"
#include "gaia/optional.hpp"

namespace gaia
{
/**
 * @addtogroup gaia
 * @{
 */
namespace expressions
{
/**
 * @addtogroup expressions
 * @{
 */

namespace detail
{

// Tags for the different AST components.
struct expression_tag_t
{
};
struct string_value_tag_t // String, const char*.
{
};
struct value_tag_t // Generic native value (e.g. int).
{
};

expression_tag_t ast_tag(const expression_base_t&);
string_value_tag_t ast_tag(const char*);
value_tag_t ast_tag(...);

// Value tag test.
// This is for a C++11 compatible version of is_value() since std::disjunction, std::negation is not implemented.
std::true_type value_tag_test(string_value_tag_t);
std::true_type value_tag_test(value_tag_t);
std::false_type value_tag_test(...);

// Get the return type of the object once it is bound and evaluated.
// The return of query_operators and expressions will be the return of the evaluated operator type.
template <typename T_bind, typename T_return>
T_return eval_type_test(const expression_t<T_bind, T_return>&, expression_tag_t);

// Specialization for string: we use std::string as it provides storage so we can avoid segfaults.
template <typename T_class>
std::string eval_type_test(T_class, string_value_tag_t);

// Default value type.
template <typename T_class>
typename std::decay<T_class>::type eval_type_test(T_class, value_tag_t);
} // namespace detail

// Infer the evaluated types for the objects when building an AST tree.
// Expressions i.e eval_type<expression<T_bind, T_return>> will evaluate to T_return
// Value types will evaluate to themselves.
// The exception will be string values e.g eval_type<char[13]> will evaluate to std::string.
template <typename T_class>
using eval_type = decltype(detail::eval_type_test(std::declval<T_class>(), detail::ast_tag(std::declval<T_class>())));

namespace detail
{

// Expression bind type tests.
// Rules are as follows:
// expression_t<T_bind, ...> -> T_bind
// Everything else -> void. We cannot infer the bind context outside of the expression context.
// For multiple template parameters, combine all the bind types of the parameters,
// Error out if we find conflicting bind contexts (i.e T_left, T_right are both non-void and of different types).

template <typename T_bind, typename T_return>
T_bind bind_type_test(const expression_t<T_bind, T_return>&);
void bind_type_test(...);

template <typename T_left, typename T_right>
struct combine_bind_t
{
    static_assert(std::is_same<T_left, T_right>::value, "Conflicting bind contexts detected.");
    using type = T_left;
};

template <typename T_left>
struct combine_bind_t<T_left, void>
{
    using type = T_left;
};

template <typename T_right>
struct combine_bind_t<void, T_right>
{
    using type = T_right;
};

template <>
struct combine_bind_t<void, void>
{
    using type = void;
};

template <typename... T_class>
struct bind_type_t;

template <typename T_first, typename... T_rest>
struct bind_type_t<T_first, T_rest...>
{
    using type = typename combine_bind_t<
        typename bind_type_t<T_first>::type,
        typename bind_type_t<T_rest...>::type>::type;
};

template <typename T_class>
struct bind_type_t<T_class>
{
    using type = decltype(bind_type_test(std::declval<T_class>()));
};

} // namespace detail

// Infer the bind type based on all the parameters.
template <typename... T_class>
using bind_type = typename detail::bind_type_t<T_class...>::type;

// Check if a class is a gaia expression.
template <typename T_class>
using is_expression = typename std::is_base_of<expression_base_t, T_class>;

// Check if a class should be treated as a value.
template <typename T_class>
using is_value = decltype(detail::value_tag_test(detail::ast_tag(std::declval<T_class>())));

/**@}*/
} // namespace expressions
/**@}*/
} // namespace gaia
