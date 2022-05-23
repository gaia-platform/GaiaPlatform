////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

// This is a meta-header containing the complete headers for building/evaluating expressions.
//
// The expression infrastructure consists of the following components:
//
// 1) Accessors - Expression AST fragments created during code generation. This represents
//    a proxy of a field to an object.
// 2) Expression builders - These are operator overloads and objects that will combine AST
//    fragments together into an executable expression tree.
// 3) Execution/Evaluation - All expression objects are also functors accepting a bind context that
//    will cause the expression to be evaluated against that context.

#include "gaia/internal/expressions/builders/expression_builder.hpp"
#include "gaia/internal/expressions/builders/expression_int_type.hpp"
#include "gaia/internal/expressions/builders/expression_optional.hpp"
#include "gaia/internal/expressions/builders/expression_string.hpp"
