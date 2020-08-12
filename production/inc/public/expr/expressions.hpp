/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>
#include "operators.hpp"
#include "param.hpp"

// Lightweight header containing forward declaration for expression classes
// Use this header to implement additional native type support beyond what is generically available

namespace gaia {
namespace expr {

template <typename T_interp>
class interpreter;

struct expr_base_t;

template <typename T>
class expression_t;

template<typename T_from, typename T_to>
class cast_expression_t;

template <typename T, typename U, typename R, typename OP>
class binary_expression_t;

template <typename T, typename U, typename T_stored, typename R, typename OP>
class binary_expression_native_storage_t;

template <typename T, typename R, typename OP>
class unary_expression_t;

template <typename T>
class native_expression_t;

template <typename T, typename T_interp>
class variable_expression_t;

template <typename R, typename F, typename ...ArgExprs>
class function_call_expression_t;


template <typename T>
using is_expr = std::is_base_of<expr_base_t, typename std::decay<T>::type>;

namespace detail {
template <typename T, bool>
struct expr_type_helper;

template<typename T>
struct expr_type_helper<T, true> {
    typedef typename std::decay<T>::type::value_type type;
};

template<typename T>
struct expr_type_helper<T, false> {
    typedef T type;
};
}

template<typename T>
using expr_type = typename detail::expr_type_helper<T, is_expr<T>::value>::type;

}
}
