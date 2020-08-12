#include "expressions.hpp"
#include "expr_base.hpp"
#include "param.hpp"

#pragma once

namespace gaia {
namespace expr {

// EQ
template<typename T, typename U>
binary_expression_t<T, U, eq_type<T, U>, OP_EQ> operator==(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, eq_type<T, U>, OP_EQ>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, eq_type<T, U>, OP_EQ> operator==(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, eq_type<T, U>, OP_EQ>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, eq_type<T, U>, OP_EQ> operator==(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, eq_type<T, U>, OP_EQ>(op1, op2);
}

// NE
template<typename T, typename U>
binary_expression_t<T, U, neq_type<T, U>, OP_NEQ> operator!=(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, neq_type<T, U>, OP_NEQ>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, neq_type<T, U>, OP_NEQ> operator!=(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, neq_type<T, U>, OP_NEQ>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, neq_type<T, U>, OP_NEQ> operator!=(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, neq_type<T, U>, OP_NEQ>(op1, op2);
}

// GT
template<typename T, typename U>
binary_expression_t<T, U, gt_type<T, U>, OP_GT> operator>(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, gt_type<T, U>, OP_GT>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, gt_type<T, U>, OP_GT> operator>(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, gt_type<T, U>, OP_GT>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, gt_type<T, U>, OP_GT> operator>(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, gt_type<T, U>, OP_GT>(op1, op2);
}

// GE
template<typename T, typename U>
binary_expression_t<T, U, ge_type<T, U>, OP_GE> operator>=(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, ge_type<T, U>, OP_GE>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, ge_type<T, U>, OP_GE> operator>=(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, ge_type<T, U>, OP_GE>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, ge_type<T, U>, OP_GE> operator>=(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, ge_type<T, U>, OP_GE>(op1, op2);
}

// LT
template<typename T, typename U>
binary_expression_t<T, U, lt_type<T, U>, OP_LT> operator<(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, lt_type<T, U>, OP_LT>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, lt_type<T, U>, OP_LT> operator<(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, lt_type<T, U>, OP_LT>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, lt_type<T, U>, OP_LT> operator<(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, lt_type<T, U>, OP_LT>(op1, op2);
}

// LE
template<typename T, typename U>
 binary_expression_t<T, U, le_type<T, U>, OP_LE> operator<=(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, le_type<T, U>, OP_LE>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, le_type<T, U>, OP_LE> operator<=(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, le_type<T, U>, OP_LE>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, le_type<T, U>, OP_LE> operator<=(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, le_type<T, U>, OP_LE>(op1, op2);
}

// AND
template<typename T, typename U>
 binary_expression_t<T, U, and_type<T, U>, OP_AND> operator&&(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, and_type<T, U>, OP_AND>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, and_type<T, U>, OP_AND> operator&&(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, and_type<T, U>, OP_AND>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, and_type<T, U>, OP_AND> operator&&(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, and_type<T, U>, OP_AND>(op1, op2);
}

// OR
template<typename T, typename U>
 binary_expression_t<T, U, nor_type<T, U>, OP_NOR> operator||(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, nor_type<T, U>, OP_NOR>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, nor_type<T, U>, OP_NOR> operator||(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, nor_type<T, U>, OP_NOR>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, nor_type<T, U>, OP_NOR> operator||(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, nor_type<T, U>, OP_NOR>(op1, op2);
}

// XOR
template<typename T, typename U>
 binary_expression_t<T, U, xor_type<T, U>, OP_XOR> operator^(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, xor_type<T, U>, OP_XOR>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, xor_type<T, U>, OP_XOR> operator^(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, xor_type<T, U>, OP_XOR>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, xor_type<T, U>, OP_XOR> operator^(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, xor_type<T, U>, OP_XOR>(op1, op2);
}

// ADD
template<typename T, typename U>
 binary_expression_t<T, U, add_type<T, U>, OP_ADD> operator+(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, add_type<T, U>, OP_ADD>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, add_type<T, U>, OP_ADD> operator+(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, add_type<T, U>, OP_ADD>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, add_type<T, U>, OP_ADD> operator+(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, add_type<T, U>, OP_ADD>(op1, op2);
}

// SUB
template<typename T, typename U>
 binary_expression_t<T, U, sub_type<T, U>, OP_SUB> operator-(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, sub_type<T, U>, OP_SUB>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, sub_type<T, U>, OP_SUB> operator-(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, sub_type<T, U>, OP_SUB>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, sub_type<T, U>, OP_SUB> operator-(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, sub_type<T, U>, OP_SUB>(op1, op2);
}

// MUL
template<typename T, typename U>
 binary_expression_t<T, U, mul_type<T, U>, OP_MUL> operator*(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, mul_type<T, U>, OP_MUL>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, mul_type<T, U>, OP_MUL> operator*(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, mul_type<T, U>, OP_MUL>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, mul_type<T, U>, OP_MUL> operator*(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, mul_type<T, U>, OP_MUL>(op1, op2);
}

// DIV
template<typename T, typename U>
 binary_expression_t<T, U, div_type<T, U>, OP_DIV> operator/(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, div_type<T, U>, OP_DIV>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, div_type<T, U>, OP_DIV> operator/(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, div_type<T, U>, OP_DIV>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, div_type<T, U>, OP_DIV> operator/(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, div_type<T, U>, OP_DIV>(op1, op2);
}

// MOD
template<typename T, typename U>
 binary_expression_t<T, U, mod_type<T, U>, OP_MOD> operator%(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, mod_type<T, U>, OP_MOD>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, mod_type<T, U>, OP_MOD> operator%(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, mod_type<T, U>, OP_MOD>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, mod_type<T, U>, OP_MOD> operator%(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, mod_type<T, U>, OP_AND>(op1, op2);
}

// BITWISE_AND
template<typename T, typename U>
 binary_expression_t<T, U, band_type<T, U>, OP_BAND> operator&(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, band_type<T, U>, OP_BAND>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, band_type<T, U>, OP_BAND> operator&(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, band_type<T, U>, OP_BAND>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, band_type<T, U>, OP_BAND> operator&(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, band_type<T, U>, OP_BAND>(op1, op2);
}

// BITWISE_OR
template<typename T, typename U>
 binary_expression_t<T, U, bor_type<T, U>, OP_BOR> operator&(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, bor_type<T, U>, OP_BOR>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, bor_type<T, U>, OP_BOR> operator&(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, bor_type<T, U>, OP_BOR>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, bor_type<T, U>, OP_BOR> operator&(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, bor_type<T, U>, OP_BOR>(op1, op2);
}

// SHL
template<typename T, typename U>
 binary_expression_t<T, U, shl_type<T, U>, OP_SHL> operator<<(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, shl_type<T, U>, OP_SHL>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, shl_type<T, U>, OP_SHL> operator<<(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, shl_type<T, U>, OP_SHL>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, shl_type<T, U>, OP_SHL> operator<<(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, shl_type<T, U>, OP_SHL>(op1, op2);
}

// SHR
template<typename T, typename U>
 binary_expression_t<T, U, shl_type<T, U>, OP_SHL> operator>>(
    const expression_t<T>& op1,
    const expression_t<U>& op2
){
    return binary_expression_t<T, U, shl_type<T, U>, OP_SHL>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, T>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, T, shr_type<T, U>, OP_SHR> operator>>(
    const T& op1,
    const expression_t<U>& op2
){
    return binary_expression_native_storage_t<T, U, T, shr_type<T, U>, OP_SHR>(op1, op2);
}

template<typename T, typename U,
        typename std::enable_if<!std::is_base_of<expr_base_t, U>::value>::type* = nullptr>
binary_expression_native_storage_t<T, U, U, shr_type<T, U>, OP_SHR> operator>>(
    const expression_t<T>& op1,
    const U& op2
){
    return binary_expression_native_storage_t<T, U, U, shr_type<T, U>, OP_SHR>(op1, op2);
}

// NEG
template <typename T>
unary_expression_t<T, neg_type<T>, OP_NEG> operator-(const expression_t<T>& operand) {
    return unary_expression_t<T, neg_type<T>, OP_NEG>(operand);
}

// NOT
template <typename T>
unary_expression_t<T, not_type<T>, OP_NOT> operator!(const expression_t<T>& operand) {
    return unary_expression_t<T, not_type<T>, OP_NOT>(operand);
}

// INV
template <typename T>
unary_expression_t<T, inv_type<T>, OP_INV> operator~(const expression_t<T>& operand) {
    return unary_expression_t<T, inv_type<T>, OP_INV>(operand);
}

}
}
