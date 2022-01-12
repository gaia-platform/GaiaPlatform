/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <type_traits>

#include "expression_base.hpp"

namespace gaia
{
namespace expressions
{

// Check if a class is a gaia expression.
template <typename T_bind>
using is_expression = typename std::is_base_of<expression_base_t, T_bind>;

/*
 * Operator tokens recognized by the C++ compiler.
 *
 * These tokens are not actual operations themselves.
 * These tokens are to specialize operator expressions so
 * query processor can run analysis and do expression rewrites
 * (e.g conversion of expressions to disjunctive normal form).
 */

enum class operator_token_t
{
    e_eq, // == token.
    e_ne, // != token.
    e_gt, // > token.
    e_ge, // >= token.
    e_lt, // < token.
    e_le, // <= token.
    e_and, // && token.
    e_or, // || token.
    e_xor, // ^ token.
    e_add, // + token.
    e_sub, // - token (binary).
    e_mul, // * token.
    e_div, // / token.
    e_mod, // % token.
    e_band, // & token.
    e_bor, // | token.
    e_shl, // << token.
    e_shr, // >> token.
    e_neg, // - token (unary).
    e_not, // ! token.
    e_inv // ~ token.
};

// Template types for operations.
//
// T_left - Left operand of binary expression.
// T_right - Right operand of binary expression.
// T_operand - Operand of unary expression.
//
// These templates resolve to the types after performing the operation
// as determined by the C++ compiler e.g. bool && bool will resolve to bool.

template <typename T_left, typename T_right>
using eq_type = decltype(std::declval<T_left&>() == std::declval<T_right&>());
template <typename T_left, typename T_right>
using ne_type = decltype(std::declval<T_left&>() != std::declval<T_right&>());
template <typename T_left, typename T_right>
using gt_type = decltype(std::declval<T_left&>() > std::declval<T_right&>());
template <typename T_left, typename T_right>
using ge_type = decltype(std::declval<T_left&>() >= std::declval<T_right&>());
template <typename T_left, typename T_right>
using lt_type = decltype(std::declval<T_left&>() < std::declval<T_right&>());
template <typename T_left, typename T_right>
using le_type = decltype(std::declval<T_left&>() <= std::declval<T_right&>());
template <typename T_left, typename T_right>
using and_type = decltype(std::declval<T_left&>() && std::declval<T_right&>());
template <typename T_left, typename T_right>
using or_type = decltype(std::declval<T_left&>() || std::declval<T_right&>());
template <typename T_left, typename T_right>
using xor_type = decltype(std::declval<T_left&>() ^ std::declval<T_right&>());
template <typename T_left, typename T_right>
using add_type = decltype(std::declval<T_left&>() + std::declval<T_right&>());
template <typename T_left, typename T_right>
using sub_type = decltype(std::declval<T_left&>() - std::declval<T_right&>());
template <typename T_left, typename T_right>
using mul_type = decltype(std::declval<T_left&>() * std::declval<T_right&>());
template <typename T_left, typename T_right>
using div_type = decltype(std::declval<T_left&>() / std::declval<T_right&>());
template <typename T_left, typename T_right>
using mod_type = decltype(std::declval<T_left&>() % std::declval<T_right&>());
template <typename T_left, typename T_right>
using band_type = decltype(std::declval<T_left&>() & std::declval<T_right&>());
template <typename T_left, typename T_right>
using bor_type = decltype(std::declval<T_left&>() | std::declval<T_right&>());
template <typename T_left, typename T_right>
using shl_type = decltype(std::declval<T_left&>() << std::declval<T_right&>());
template <typename T_left, typename T_right>
using shr_type = decltype(std::declval<T_left&>() >> std::declval<T_right&>());

template <typename T_operand>
using neg_type = decltype(-std::declval<T_operand&>());
template <typename T_operand>
using not_type = decltype(!std::declval<T_operand&>());
template <typename T_operand>
using inv_type = decltype(~std::declval<T_operand&>());

} // namespace expressions
} // namespace gaia
