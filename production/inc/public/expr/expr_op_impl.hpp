/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stdexcept>
#include "operators.hpp"

// Actual methods called by eval().
// These call the native type's defined operator.

namespace gaia {
namespace expr {

template<typename T, typename U>
eq_type<T,U> eval_eq(const T& lhs, const U& rhs) {
    return lhs == rhs;
}

template<typename T, typename U>
eq_type<T,U> eval_eq(T& lhs, U& rhs) {
    return lhs == rhs;
}

template<typename T, typename U>
neq_type<T,U> eval_neq(const T& lhs, const U& rhs) {
    return lhs != rhs;
}

template<typename T, typename U>
neq_type<T,U> eval_neq(T& lhs, U& rhs) {
    return lhs != rhs;
}

template<typename T, typename U>
gt_type<T,U> eval_gt(const T& lhs, const U& rhs) {
    return lhs > rhs;
}

template<typename T, typename U>
gt_type<T,U> eval_gt(T& lhs, U& rhs) {
    return lhs > rhs;
}

template<typename T, typename U>
ge_type<T,U> eval_ge(const T& lhs, const U& rhs) {
    return lhs >= rhs;
}

template<typename T, typename U>
ge_type<T,U> eval_ge(T& lhs, U& rhs) {
    return lhs >= rhs;
}

template<typename T, typename U>
lt_type<T,U> eval_lt(const T& lhs, const U& rhs) {
    return lhs < rhs;
}

template<typename T, typename U>
lt_type<T,U> eval_lt(T& lhs, U& rhs) {
    return lhs < rhs;
}

template<typename T, typename U>
le_type<T,U> eval_le(const T& lhs, const U& rhs) {
    return lhs <= rhs;
}

template<typename T, typename U>
le_type<T,U> eval_le(T& lhs, U& rhs) {
    return lhs <= rhs;
}

template<typename T, typename U>
and_type<T,U> eval_and(const T& lhs, const U& rhs) {
    return lhs && rhs;
}

template<typename T, typename U>
and_type<T,U> eval_and(T& lhs, U& rhs) {
    return lhs && rhs;
}

template<typename T, typename U>
nor_type<T,U> eval_nor(const T& lhs, const U& rhs) {
    return lhs || rhs;
}

template<typename T, typename U>
nor_type<T,U> eval_nor(T& lhs, U& rhs) {
    return lhs || rhs;
}

template<typename T, typename U>
xor_type<T,U> eval_xor(const T& lhs, const U& rhs) {
    return lhs ^ rhs;
}

template<typename T, typename U>
xor_type<T,U> eval_xor(T& lhs, U& rhs) {
    return lhs ^ rhs;
}

template<typename T, typename U>
add_type<T,U> eval_add(const T& lhs, const U& rhs) {
    return lhs + rhs;
}

template<typename T, typename U>
add_type<T,U> eval_add(T& lhs, U& rhs) {
    return lhs + rhs;
}

template<typename T, typename U>
sub_type<T,U> eval_sub(const T& lhs, const U& rhs) {
    return lhs - rhs;
}

template<typename T, typename U>
sub_type<T,U> eval_sub(T& lhs, U& rhs) {
    return lhs - rhs;
}

template<typename T, typename U>
add_type<T,U> eval_mul(const T& lhs, const U& rhs) {
    return lhs * rhs;
}

template<typename T, typename U>
add_type<T,U> eval_mul(T& lhs, U& rhs) {
    return lhs * rhs;
}

template<typename T, typename U>
sub_type<T,U> eval_div(const T& lhs, const U& rhs) {
    return lhs / rhs;
}

template<typename T, typename U>
sub_type<T,U> eval_div(T& lhs, U& rhs) {
    return lhs / rhs;
}

template<typename T, typename U>
mod_type<T,U> eval_mod(const T& lhs, const U& rhs) {
    return lhs % rhs;
}

template<typename T, typename U>
mod_type<T,U> eval_mod(T& lhs, U& rhs) {
    return lhs % rhs;
}

template<typename T, typename U>
band_type<T,U> eval_band(const T& lhs, const U& rhs) {
    return lhs & rhs;
}

template<typename T, typename U>
band_type<T,U> eval_band(T& lhs, U& rhs) {
    return lhs & rhs;
}

template<typename T, typename U>
bor_type<T,U> eval_bor(const T& lhs, const U& rhs) {
    return lhs | rhs;
}

template<typename T, typename U>
bor_type<T,U> eval_bor(T& lhs, U& rhs) {
    return lhs | rhs;
}

template<typename T, typename U>
shl_type<T,U> eval_shl(const T& lhs, const U& rhs) {
    return lhs << rhs;
}

template<typename T, typename U>
shl_type<T,U> eval_shl(T& lhs, U& rhs) {
    return lhs << rhs;
}

template<typename T, typename U>
shr_type<T,U> eval_shr(const T& lhs, const U& rhs) {
    return lhs >> rhs;
}

template<typename T, typename U>
shr_type<T,U> eval_shr(T& lhs, U& rhs) {
    return lhs >> rhs;
}

template<typename T>
neg_type<T> eval_neg(const T& operand) {
    return -operand;
}

template<typename T>
neg_type<T> eval_neg(T& operand) {
    return -operand;
}

template<typename T>
not_type<T> eval_not(const T& operand) {
    return !operand;
}

template<typename T>
not_type<T> eval_not(T& operand) {
    return !operand;
}

template<typename T>
inv_type<T> eval_inv(const T& operand) {
    return ~operand;
}

template<typename T>
inv_type<T> eval_inv(T& operand) {
    return ~operand;
}

}
}
