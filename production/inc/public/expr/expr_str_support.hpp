/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <type_traits>

#include "expressions.hpp"
#include "expr_base.hpp"
#include "expr_binary.hpp"
#include "expr_native.hpp"

// This file adds additional support for the std::string native type.
// This is to allow expressions to work with various string equivalent types and literals.
//
// As of C++20, the following operators have been deprecated !=, <, >, <=, >=
// This class readds that support for predicate expressions

namespace gaia {
namespace expr {

template<typename T>
using treat_as_string = std::is_assignable<std::string, T>;

// Additional AST construction templates for string

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_EQ> operator==(
    const T& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_EQ> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_EQ> operator==(
    const expression_t<std::string>& lhs,
    const T& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_EQ>(lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_EQ> operator==(
    const std::string& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_EQ> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_EQ> operator==(
    const expression_t<std::string>& lhs,
    const std::string& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_EQ> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_NEQ> operator!=(
    const T& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_NEQ> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_NEQ> operator!=(
    const expression_t<std::string>& lhs,
    const T& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_NEQ>(lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_NEQ> operator!=(
    const std::string& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_NEQ> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_NEQ> operator!=(
    const expression_t<std::string>& lhs,
    const std::string& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_NEQ> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GT> operator>(
    const T& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GT> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GT> operator>(
    const expression_t<std::string>& lhs,
    const T& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GT> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GT> operator>(
    const std::string& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GT> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GT> operator>(
    const expression_t<std::string>& lhs,
    const std::string& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GT> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GE> operator>=(
    const T& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GE> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GE> operator>=(
    const expression_t<std::string>& lhs,
    const T& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GE> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GE> operator>=(
    const std::string& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GE> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GE> operator>=(
    const expression_t<std::string>& lhs,
    const std::string& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_GE> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LT>operator<(
    const T& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LT> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LT> operator<(
    const expression_t<std::string>& lhs,
    const T& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LT> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LT> operator<(
    const std::string& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LT> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LT> operator<(
    const expression_t<std::string>& lhs,
    const std::string& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LT> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LE> operator<=(
    const T& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LE> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LE> operator<=(
    const expression_t<std::string>& lhs,
    const T& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LE> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LE> operator<=(
    const std::string& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LE> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LE> operator<=(
    const expression_t<std::string>& lhs,
    const std::string& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, bool, OP_LE> (lhs, rhs);
}

template<typename T,
        typename std::enable_if<treat_as_string<T>::value>::type* = nullptr>
binary_expression_native_storage_t<std::string, std::string, std::string, std::string&, OP_ADD> operator+(
    const expression_t<std::string>& lhs,
    const T& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, std::string&, OP_ADD> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, std::string&, OP_ADD> operator+(
    const std::string& lhs,
    const expression_t<std::string>& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, std::string&, OP_ADD> (lhs, rhs);
}

template<typename T_gaia>
binary_expression_native_storage_t<std::string, std::string, std::string, std::string&, OP_ADD> operator+(
    const expression_t<std::string>& lhs,
    const std::string& rhs) {
    return binary_expression_native_storage_t<std::string, std::string, std::string, std::string&, OP_ADD> (lhs, rhs);
}

// Override eval_xx() via tag dispatch (C++20 compatibility)
template <typename T, typename U>
bool eval_eq(const std::string& lhs, const std::string& rhs) {
    return lhs.compare(rhs) == 0;
}

template <typename T, typename U>
bool eval_neq(const std::string& lhs, const std::string& rhs) {
    return lhs.compare(rhs) != 0;
}

template <typename T, typename U>
bool eval_gt(const std::string& lhs, const std::string& rhs) {
    return lhs.compare(rhs) > 0;
}

template <typename T, typename U>
bool eval_ge(const std::string& lhs, const std::string& rhs) {
    return lhs.compare(rhs) >= 0;
}

template <typename T, typename U>
bool eval_lt(const std::string& lhs, const std::string& rhs) {
    return lhs.compare(rhs) < 0;
}

template <typename T, typename U>
bool eval_le(const std::string& lhs, const std::string& rhs) {
    return lhs.compare(rhs) <= 0;
}

template <typename T, typename U>
std::string& eval_add(std::string& lhs, const std::string& rhs) {
    return lhs.append(rhs);
}

}
}
