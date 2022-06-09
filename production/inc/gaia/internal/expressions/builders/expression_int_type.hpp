////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"
#include "gaia/expressions/operators.hpp"

// Operations to allow queries on Gaia's accessors derived from int_type.
//
// Currently only common::gaia_id_t operations are defined.
//
// We only support == and != operators for queries.

namespace gaia
{
namespace expressions
{

// == operator.
inline constexpr bool evaluate_operator(common::gaia_id_t left, common::gaia_id_t right, operator_eq_t)
{
    return left.value() == right.value();
}

// != operator.
inline constexpr bool evaluate_operator(common::gaia_id_t left, common::gaia_id_t right, operator_ne_t)
{
    return left.value() != right.value();
}

} // namespace expressions
} // namespace gaia
