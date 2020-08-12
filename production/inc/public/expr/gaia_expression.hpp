/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "ast_builder.hpp"
#include "interpreter.hpp"
#include "expr_str_support.hpp" // this needs to be above the others!
#include "expr_base.hpp"
#include "expr_cast.hpp"
#include "expr_unary.hpp"
#include "expr_binary.hpp"
#include "expr_native.hpp"
#include "expr_variable.hpp"
#include "expr_field_access.hpp"
#include "expr_array_access.hpp"
#include "expr_fn_call.hpp"

// This file declares a set of classes that constitute an interpretable AST
// Use in conjunction with codegen to provide a embedded DSL
// Unfortunately, failed AST construction does result in ugly compile errors due to templates
