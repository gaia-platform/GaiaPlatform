/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <cstring>

#include <string>

#include "gaia/expressions/expression_traits.hpp"
#include "gaia/expressions/operators.hpp"

// This file supports additional string operations using template specialization.
// Specifically, we support equals and not equals comparisons against const char*
// types in C++. <string> already defines operator== in C++11 but != was removed in C++20, so we define
// that here.

namespace gaia
{
namespace expressions
{

// Actual operator evaluation implementations.
static inline bool evaluate_operator(const char* left, const char* right, operator_eq_t)
{
    return strcmp(left, right) == 0;
}

static inline bool evaluate_operator(const char* left, const char* right, operator_ne_t)
{
    return strcmp(left, right) != 0;
}

static inline bool evaluate_operator(const std::string& left, const char* right, operator_ne_t)
{
    return strcmp(left.c_str(), right) != 0;
}

static inline bool evaluate_operator(const char* left, const std::string& right, operator_ne_t)
{
    return strcmp(left, right.c_str()) != 0;
}

} // namespace expressions
} // namespace gaia
