/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

/*
 * This is a meta-header containing the complete headers for building/evaluating expressions.
 *
 * The expression infrastructure consists of the following components:
 *
 * 1) Accessors - Expression AST fragments created during code generation. This represents
 *    a proxy to a field to an object.
 * 2) Expression builders - These are operator overloads and objects that will combine AST
 *    fragments together into an executable expression tree.
 * 3) Execution/Evaluation - All expression objects are also functors accepting a bind context that
 *    will cause the expression evaluated against that context.
 */

#include "expression_builder.hpp"
#include "expression_int_type.hpp"
#include "expression_string.hpp"
