/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <type_traits>

#include "expression_ast.hpp"
#include "expression_traits.hpp"

// This file builds ASTs supporting additional string operations using template specialization.
// Specifically, we support equals and not equals comparisons against const char* and std::string
// types in C++.

namespace gaia
{
namespace expressions
{

// == operator.
template <typename T_bind>
binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_eq>
operator==(
    const expression_t<T_bind, const char*>& left,
    const expression_t<T_bind, const char*>& right)
{
    return binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_eq>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, const char*>& left_expression,
            const subexpression_t<T_bind, const char*>& right_expression) {
            return strcmp((*left_expression)(bind), (*right_expression)(bind)) == 0;
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, std::string, const char*, operator_token_t::e_eq>
operator==(
    const std::string& left,
    const expression_t<T_bind, const char*>& right)
{
    return binary_expression_t<T_bind, bool, std::string, const char*, operator_token_t::e_eq>(
        static_cast<value_accessor_t<T_bind, std::string>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, std::string>&,
            const subexpression_t<T_bind, const char*>& right_expression) {
            return strcmp(left.c_str(), (*right_expression)(bind)) == 0;
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, const char*, std::string, operator_token_t::e_eq>
operator==(
    const expression_t<T_bind, const char*>& left,
    const std::string& right)
{
    return binary_expression_t<T_bind, bool, const char*, std::string, operator_token_t::e_eq>(
        left, static_cast<value_accessor_t<T_bind, std::string>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, const char*>& left_expression,
            const subexpression_t<T_bind, std::string>&) {
            return strcmp((*left_expression)(bind), right.c_str()) == 0;
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_eq>
operator==(
    const char* left,
    const expression_t<T_bind, const char*>& right)
{
    return binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_eq>(
        static_cast<value_accessor_t<T_bind, const char*>>(left), right,
        [left](
            const T_bind& bind,
            const subexpression_t<T_bind, const char*>&,
            const subexpression_t<T_bind, const char*>& right_expression) {
            return strcmp(left, (*right_expression)(bind)) == 0;
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_eq>
operator==(
    const expression_t<T_bind, const char*>& left,
    const char* right)
{
    return binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_eq>(
        left, static_cast<value_accessor_t<T_bind, const char*>>(right),
        [right](
            const T_bind& bind,
            const subexpression_t<T_bind, const char*>& left_expression,
            const subexpression_t<T_bind, const char*>&) {
            return strcmp((*left_expression)(bind), right) == 0;
        });
}

// != operator.
template <typename T_bind>
binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_ne>
operator!=(
    const expression_t<T_bind, const char*>& left,
    const expression_t<T_bind, const char*>& right)
{
    return binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_ne>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, const char*>& left_expression,
            const subexpression_t<T_bind, const char*>& right_expression) {
            return strcmp((*left_expression)(bind), (*right_expression)(bind)) != 0;
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, std::string, const char*, operator_token_t::e_ne>
operator!=(
    const std::string& left,
    const expression_t<T_bind, const char*>& right)
{
    return binary_expression_t<T_bind, bool, std::string, const char*, operator_token_t::e_ne>(
        static_cast<value_accessor_t<T_bind, std::string>>(left), right,
        [&left](
            const T_bind& bind,
            const subexpression_t<T_bind, std::string>&,
            const subexpression_t<T_bind, const char*>& right_expression) {
            return strcmp(left.c_str(), (*right_expression)(bind)) != 0;
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, const char*, std::string, operator_token_t::e_ne>
operator!=(
    const expression_t<T_bind, const char*>& left,
    const std::string& right)
{
    return binary_expression_t<T_bind, bool, const char*, std::string, operator_token_t::e_ne>(
        left, static_cast<value_accessor_t<T_bind, std::string>>(right),
        [&right](
            const T_bind& bind,
            const subexpression_t<T_bind, const char*>& left_expression,
            const subexpression_t<T_bind, std::string>&) {
            return strcmp((*left_expression)(bind), right.c_str()) != 0;
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_ne>
operator!=(
    const char* left,
    const expression_t<T_bind, const char*>& right)
{
    return binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_ne>(
        static_cast<value_accessor_t<T_bind, const char*>>(left), right,
        [left](
            const T_bind& bind,
            const subexpression_t<T_bind, const char*>&,
            const subexpression_t<T_bind, const char*>& right_expression) {
            return strcmp(left, (*right_expression)(bind)) != 0;
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_ne>
operator!=(
    const expression_t<T_bind, const char*>& left,
    const char* right)
{
    return binary_expression_t<T_bind, bool, const char*, const char*, operator_token_t::e_ne>(
        left, static_cast<value_accessor_t<T_bind, const char*>>(right),
        [right](
            const T_bind& bind,
            const subexpression_t<T_bind, const char*>& left_expression,
            const subexpression_t<T_bind, const char*>&) {
            return strcmp((*left_expression)(bind), right) != 0;
        });
}

} // namespace expressions
} // namespace gaia
