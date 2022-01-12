/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>

#include "gaia/common.hpp"

#include "expression_ast.hpp"
/*
 * Operations to allow queries on Gaia's accessors derived from int_type.
 *
 * Currently only common::gaia_id_t operations are defined.
 *
 * We only support == and != operators for queries.
 */

namespace gaia
{
namespace expressions
{
// == operator.
template <typename T_bind>
binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_eq>
operator==(const expression_t<T_bind, common::gaia_id_t>& left, const expression_t<T_bind, common::gaia_id_t>& right)
{
    return binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_eq>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, common::gaia_id_t>& left_expression,
            const subexpression_t<T_bind, common::gaia_id_t>& right_expression) {
            return static_cast<common::gaia_id_t::value_type>((*left_expression)(bind))
                == static_cast<common::gaia_id_t::value_type>((*right_expression)(bind));
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_eq>
operator==(const expression_t<T_bind, common::gaia_id_t>& left, common::gaia_id_t right)
{
    return binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_eq>(
        left, static_cast<value_accessor_t<T_bind, common::gaia_id_t>>(right),
        [right](
            const T_bind& bind,
            const subexpression_t<T_bind, common::gaia_id_t>& left_expression,
            const subexpression_t<T_bind, common::gaia_id_t>&) {
            return static_cast<common::gaia_id_t::value_type>((*left_expression)(bind))
                == static_cast<common::gaia_id_t::value_type>(right);
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_eq>
operator==(common::gaia_id_t left, const expression_t<T_bind, common::gaia_id_t>& right)
{
    return binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_eq>(
        static_cast<value_accessor_t<T_bind, common::gaia_id_t>>(left), right,
        [left](
            const T_bind& bind,
            const subexpression_t<T_bind, common::gaia_id_t>&,
            const subexpression_t<T_bind, common::gaia_id_t>& right_expression) {
            return static_cast<common::gaia_id_t::value_type>(left)
                == static_cast<common::gaia_id_t::value_type>((*right_expression)(bind));
        });
}

// != operator
template <typename T_bind>
binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_ne>
operator!=(const expression_t<T_bind, common::gaia_id_t>& left, const expression_t<T_bind, common::gaia_id_t>& right)
{
    return binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_ne>(
        left, right,
        [](
            const T_bind& bind,
            const subexpression_t<T_bind, common::gaia_id_t>& left_expression,
            const subexpression_t<T_bind, common::gaia_id_t>& right_expression) {
            return static_cast<common::gaia_id_t::value_type>((*left_expression)(bind))
                != static_cast<common::gaia_id_t::value_type>((*right_expression)(bind));
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_ne>
operator!=(const expression_t<T_bind, common::gaia_id_t>& left, common::gaia_id_t right)
{
    return binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_ne>(
        left, static_cast<value_accessor_t<T_bind, common::gaia_id_t>>(right),
        [right](
            const T_bind& bind,
            const subexpression_t<T_bind, common::gaia_id_t>& left_expression,
            const subexpression_t<T_bind, common::gaia_id_t>&) {
            return static_cast<common::gaia_id_t::value_type>((*left_expression)(bind))
                != static_cast<common::gaia_id_t::value_type>(right);
        });
}

template <typename T_bind>
binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_ne>
operator!=(common::gaia_id_t left, const expression_t<T_bind, common::gaia_id_t>& right)
{
    return binary_expression_t<T_bind, bool, common::gaia_id_t, common::gaia_id_t, operator_token_t::e_ne>(
        static_cast<value_accessor_t<T_bind, common::gaia_id_t>>(left), right,
        [left](
            const T_bind& bind,
            const subexpression_t<T_bind, common::gaia_id_t>&,
            const subexpression_t<T_bind, common::gaia_id_t>& right_expression) {
            return static_cast<common::gaia_id_t::value_type>(left)
                != static_cast<common::gaia_id_t::value_type>((*right_expression)(bind));
        });
}

} // namespace expressions
} // namespace gaia
