/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <type_traits>

#pragma once

namespace gaia
{
namespace expressions
{
// Tags representing operator tokens recognized by the C++ compiler.
//
// These tokens are not actual operations themselves.
// These tokens are to specialize operator expressions so that the
// query processor can run analysis and do expression rewrites
// (e.g conversion of expressions to disjunctive normal form).

struct operator_eq_t
{
}; // == token.
struct operator_ne_t
{
}; // != token.
struct operator_gt_t
{
}; // > token.
struct operator_ge_t
{
}; // >= token.
struct operator_lt_t
{
}; // < token
struct operator_le_t
{
}; // <= token.
struct operator_and_t
{
}; // && token.
struct operator_or_t
{
}; // || token.
struct operator_xor_t
{
}; // ^ token.
struct operator_add_t
{
}; // + token.
struct operator_sub_t
{
}; // - token (binary).
struct operator_mul_t
{
}; // * token.
struct operator_div_t
{
}; // . token.
struct operator_mod_t
{
}; // % token.
struct operator_band_t
{
}; // & token.
struct operator_bor_t
{
}; // | token.
struct operator_shl_t
{
}; // << token.
struct operator_shr_t
{
}; // >> token.
struct operator_neg_t
{
}; // - token (unary).
struct operator_not_t
{
}; // !token.
struct operator_inv_t
{
}; // ~token.

// Template types for operations.
//
// T_left - Left operand of binary expression.
// T_right - Right operand of binary expression.
// T_operand - Operand of unary expression.
//
// These templates resolve to the types after performing the operation
// as determined by the C++ compiler e.g. bool && bool will resolve to bool.

template <typename T_left, typename T_right>
using eq_default_type = decltype(std::declval<T_left>() == std::declval<T_right>());
template <typename T_left, typename T_right>
using ne_default_type = decltype(std::declval<T_left>() != std::declval<T_right>());
template <typename T_left, typename T_right>
using gt_default_type = decltype(std::declval<T_left>() > std::declval<T_right>());
template <typename T_left, typename T_right>
using ge_default_type = decltype(std::declval<T_left>() >= std::declval<T_right>());
template <typename T_left, typename T_right>
using lt_default_type = decltype(std::declval<T_left>() < std::declval<T_right>());
template <typename T_left, typename T_right>
using le_default_type = decltype(std::declval<T_left>() <= std::declval<T_right>());
template <typename T_left, typename T_right>
using and_default_type = decltype(std::declval<T_left>() && std::declval<T_right>());
template <typename T_left, typename T_right>
using or_default_type = decltype(std::declval<T_left>() || std::declval<T_right>());
template <typename T_left, typename T_right>
using xor_default_type = decltype(std::declval<T_left>() ^ std::declval<T_right>());
template <typename T_left, typename T_right>
using add_default_type = decltype(std::declval<T_left>() + std::declval<T_right>());
template <typename T_left, typename T_right>
using sub_default_type = decltype(std::declval<T_left>() - std::declval<T_right>());
template <typename T_left, typename T_right>
using mul_default_type = decltype(std::declval<T_left>() * std::declval<T_right>());
template <typename T_left, typename T_right>
using div_default_type = decltype(std::declval<T_left>() / std::declval<T_right>());
template <typename T_left, typename T_right>
using mod_default_type = decltype(std::declval<T_left>() % std::declval<T_right>());
template <typename T_left, typename T_right>
using band_default_type = decltype(std::declval<T_left>() & std::declval<T_right>());
template <typename T_left, typename T_right>
using bor_default_type = decltype(std::declval<T_left>() | std::declval<T_right>());
template <typename T_left, typename T_right>
using shl_default_type = decltype(std::declval<T_left>() << std::declval<T_right>());
template <typename T_left, typename T_right>
using shr_default_type = decltype(std::declval<T_left>() >> std::declval<T_right>());

template <typename T_operand>
using neg_default_type = decltype(-std::declval<T_operand&>());
template <typename T_operand>
using not_default_type = decltype(!std::declval<T_operand&>());
template <typename T_operand>
using inv_default_type = decltype(~std::declval<T_operand&>());

// Default operator substitution evaluation templates.
// These evaluations use SFINAE to perform the default evaluation of C++ operators.

template <typename T_left, typename T_right>
static inline eq_default_type<T_left, T_right>
evaluate_operator(const T_left& left, const T_right& right, operator_eq_t)
{
    return left == right;
}

template <typename T_left, typename T_right>
static inline ne_default_type<T_left, T_right>
evaluate_operator(const T_left& left, const T_right& right, operator_ne_t)
{
    return left != right;
}

template <typename T_left, typename T_right>
static inline gt_default_type<T_left, T_right>
evaluate_operator(const T_left& left, const T_right& right, operator_gt_t)
{
    return left > right;
}

template <typename T_left, typename T_right>
static inline ge_default_type<T_left, T_right>
evaluate_operator(const T_left& left, const T_right& right, operator_ge_t)
{
    return left >= right;
}

template <typename T_left, typename T_right>
static inline lt_default_type<T_left, T_right>
evaluate_operator(const T_left& left, const T_right& right, operator_lt_t)
{
    return left < right;
}

template <typename T_left, typename T_right>
static inline ge_default_type<T_left, T_right>
evaluate_operator(const T_left& left, const T_right& right, operator_le_t)
{
    return left <= right;
}

template <typename T_left, typename T_right>
static inline and_default_type<T_left, T_right>
evaluate_operator(const T_left& left, const T_right& right, operator_and_t)
{
    return left && right;
}

template <typename T_left, typename T_right>
static inline or_default_type<T_left, T_right>
evaluate_operator(const T_left& left, const T_right& right, operator_or_t)
{
    return left || right;
}

template <typename T_left, typename T_right>
static inline xor_default_type<T_left, T_right>
evaluate_operator(const T_left& left, const T_right& right, operator_xor_t)
{
    return left ^ right;
}

template <typename T_operand>
static inline not_default_type<T_operand>
evaluate_operator(const T_operand& operand, operator_not_t)
{
    return !operand;
}

// Template definitions for type returns.
// We rely on a evaluate_operator() type being defined for the type.
template <typename T_left, typename T_right>
using eq_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_eq_t>()));
template <typename T_left, typename T_right>
using ne_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_ne_t>()));
template <typename T_left, typename T_right>
using gt_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_gt_t>()));
template <typename T_left, typename T_right>
using ge_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_ge_t>()));
template <typename T_left, typename T_right>
using lt_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_lt_t>()));
template <typename T_left, typename T_right>
using le_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_le_t>()));
template <typename T_left, typename T_right>
using and_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_and_t>()));
template <typename T_left, typename T_right>
using or_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_or_t>()));
template <typename T_left, typename T_right>
using xor_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_xor_t>()));
template <typename T_left, typename T_right>
using add_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_add_t>()));
template <typename T_left, typename T_right>
using sub_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_sub_t>()));
template <typename T_left, typename T_right>
using div_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_div_t>()));
template <typename T_left, typename T_right>
using mul_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_mul_t>()));
template <typename T_left, typename T_right>
using mod_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_mod_t>()));
template <typename T_left, typename T_right>
using band_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_band_t>()));
template <typename T_left, typename T_right>
using bor_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_bor_t>()));
template <typename T_left, typename T_right>
using shl_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_shl_t>()));
template <typename T_left, typename T_right>
using shr_type = decltype(evaluate_operator(std::declval<T_right>(), std::declval<T_left>(), std::declval<operator_shr_t>()));

template <typename T_operand>
using not_type = decltype(evaluate_operator(std::declval<T_operand>(), std::declval<operator_not_t>()));
template <typename T_operand>
using neg_type = decltype(evaluate_operator(std::declval<T_operand>(), std::declval<operator_neg_t>()));
template <typename T_operand>
using inv_type = decltype(evaluate_operator(std::declval<T_operand>(), std::declval<operator_inv_t>()));

} // namespace expressions
} // namespace gaia
