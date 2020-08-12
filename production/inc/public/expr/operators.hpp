/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>

// This file contains common boilerplate necessary for implementation of operators

namespace gaia {
namespace expr {

// These correspond to operator tokens instead of actual operations themselves
// This is so that operators on native types shall have the exact same behavior
// as that in the the underlying.
//
// Individual expr nodes implement their own eval() semantics

struct OP_EQ; // ==
struct OP_NEQ; // !=
struct OP_GT; // >
struct OP_GE; // >=
struct OP_LT; // <
struct OP_LE; // <=
struct OP_AND; // &&
struct OP_NOR; // ||
struct OP_XOR; // ^
struct OP_ADD; // +
struct OP_SUB; // binary -
struct OP_MUL; // *
struct OP_DIV; // /
struct OP_MOD; // %
struct OP_BAND; // &
struct OP_BOR; // |
struct OP_SHL; // >>
struct OP_SHR; // <<
struct OP_NOT; // !
struct OP_NEG; // unary -
struct OP_INV; // ~

// Template types for operators
template<typename T, typename U=T>
using eq_type = decltype(std::declval<T&>() == std::declval<U&>());
template<typename T, typename U=T>
using neq_type = decltype(std::declval<T&>() != std::declval<U&>());
template<typename T, typename U=T>
using gt_type = decltype(std::declval<T&>() > std::declval<U&>());
template<typename T, typename U=T>
using ge_type = decltype(std::declval<T&>() >= std::declval<U&>());
template<typename T, typename U=T>
using lt_type = decltype(std::declval<T&>() < std::declval<U&>());
template<typename T, typename U=T>
using le_type = decltype(std::declval<T&>() <= std::declval<U&>());
template<typename T, typename U=T>
using and_type = decltype(std::declval<T&>() && std::declval<U&>());
template<typename T, typename U=T>
using nor_type = decltype(std::declval<T&>() || std::declval<U&>());
template<typename T, typename U=T>
using xor_type = decltype(std::declval<T&>() ^ std::declval<U&>());
template<typename T, typename U=T>
using add_type = decltype(std::declval<T&>() + std::declval<U&>());
template<typename T, typename U=T>
using sub_type = decltype(std::declval<T&>() - std::declval<U&>());
template<typename T, typename U=T>
using mul_type = decltype(std::declval<T&>() * std::declval<U&>());
template<typename T, typename U=T>
using div_type = decltype(std::declval<T&>() / std::declval<U&>());
template<typename T, typename U=T>
using mod_type = decltype(std::declval<T&>() % std::declval<U&>());
template<typename T, typename U=T>
using band_type = decltype(std::declval<T&>() & std::declval<U&>());
template<typename T, typename U=T>
using bor_type = decltype(std::declval<T&>() | std::declval<U&>());
template<typename T, typename U=T>
using shl_type = decltype(std::declval<T&>() << std::declval<U&>());
template<typename T, typename U=T>
using shr_type = decltype(std::declval<T&>() >> std::declval<U&>());

template<typename T, typename U=T>
using neg_type = decltype(-std::declval<T&>());
template<typename T, typename U=T>
using not_type = decltype(!std::declval<T&>());
template<typename T, typename U=T>
using inv_type = decltype(~std::declval<T&>());

// Operator support tests
template<typename T, typename U=T, typename = eq_type<T,U>>
std::true_type has_eq_test(const T&, const U&);
std::false_type has_eq_test(...);
template<typename T, typename U>
using has_eq = decltype(has_eq_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = neq_type<T,U>>
std::true_type has_neq_test(const T&, const U&);
std::false_type has_neq_test(...);
template<typename T, typename U>
using has_neq = decltype(has_neq_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = gt_type<T,U>>
std::true_type has_gt_test(const T&, const U&);
std::false_type has_gt_test(...);
template<typename T, typename U>
using has_gt = decltype(has_gt_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = ge_type<T,U>>
std::true_type has_ge_test(const T&, const U&);
std::false_type has_ge_test(...);
template<typename T, typename U>
using has_ge = decltype(has_ge_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = lt_type<T,U>>
std::true_type has_lt_test(const T&, const U&);
std::false_type has_lt_test(...);
template<typename T, typename U>
using has_lt = decltype(has_lt_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = le_type<T,U>>
std::true_type has_le_test(const T&, const U&);
std::false_type has_le_test(...);
template<typename T, typename U>
using has_le = decltype(has_le_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = and_type<T,U>>
std::true_type has_and_test(const T&, const U&);
std::false_type has_and_test(...);
template<typename T, typename U>
using has_and = decltype(has_and_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = nor_type<T,U>>
std::true_type has_nor_test(const T&, const U&);
std::false_type has_nor_test(...);
template<typename T, typename U>
using has_nor = decltype(has_nor_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = xor_type<T,U>>
std::true_type has_xor_test(const T&, const U&);
std::false_type has_xor_test(...);
template<typename T, typename U>
using has_xor = decltype(has_xor_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = add_type<T,U>>
std::true_type has_add_test(const T&, const U&);
std::false_type has_add_test(...);
template<typename T, typename U>
using has_add = decltype(has_add_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = sub_type<T,U>>
std::true_type has_sub_test(const T&, const U&);
std::false_type has_sub_test(...);
template<typename T, typename U>
using has_sub = decltype(has_sub_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = mul_type<T,U>>
std::true_type has_mul_test(const T&, const U&);
std::false_type has_mul_test(...);
template<typename T, typename U>
using has_mul = decltype(has_mul_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = div_type<T,U>>
std::true_type has_div_test(const T&, const U&);
std::false_type has_div_test(...);
template<typename T, typename U>
using has_div = decltype(has_div_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = mod_type<T,U>>
std::true_type has_mod_test(const T&, const U&);
std::false_type has_mod_test(...);
template<typename T, typename U>
using has_mod = decltype(has_mod_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = band_type<T,U>>
std::true_type has_band_test(const T&, const U&);
std::false_type has_band_test(...);
template<typename T, typename U>
using has_band = decltype(has_band_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = band_type<T,U>>
std::true_type has_bor_test(const T&, const U&);
std::false_type has_bor_test(...);
template<typename T, typename U>
using has_bor = decltype(has_bor_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = shl_type<T,U>>
std::true_type has_shl_test(const T&, const U&);
std::false_type has_shl_test(...);
template<typename T, typename U>
using has_shl = decltype(has_shl_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename U=T, typename = shr_type<T,U>>
std::true_type has_shr_test(const T&, const U&);
std::false_type has_shr_test(...);
template<typename T, typename U>
using has_shr = decltype(has_shr_test(std::declval<T>(), std::declval<U>()));

template<typename T, typename = neg_type<T>>
std::true_type has_neg_test(const T&);
std::false_type has_neg_test(...);
template<typename T>
using has_neg = decltype(has_neg_test(std::declval<T>()));

template<typename T, typename = not_type<T>>
std::true_type has_not_test(const T&);
std::false_type has_not_test(...);
template<typename T>
using has_not = decltype(has_not_test(std::declval<T>()));

template<typename T, typename = inv_type<T>>
std::true_type has_inv_test(const T&);
std::false_type has_inv_test(...);
template<typename T>
using has_inv = decltype(has_inv_test(std::declval<T>()));
}
}
