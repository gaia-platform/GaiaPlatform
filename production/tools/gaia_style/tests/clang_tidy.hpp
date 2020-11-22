/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <exception>

#include "gaia/common.hpp"

/**
 * Showcase how our .clang-tidy configuration works.
 */

using namespace gaia::common;
using namespace std;

// Should have c_ prefix.
constexpr int constant_global = 2;

// Should have c_ prefix.
extern const int c_extern_constant = 6;

// Should have g_ prefix.
extern int global;

typedef long long int int_sixty_Four;

// Should be lower case.
namespace Gaia
{
// Should have _t suffix. We can't catch that because of exceptions.
class clang_tidy
{
    // Single-argument constructors must be marked explicit to avoid unintentional implicit conversions.
    clang_tidy(int _member)
        : member(_member){};

    // Wrong casing.
    void Method();

    // Argument wrong casing.
    void other_method(int BadArgument);

    // Should have implementation.
    void no_implementation();

private:
    // Should have m_ prefix.
    int member;
};

// Should have _t suffix.
struct structure
{
};

// Wrong casing.
class Ugly_t
{
};

// Wrong casing.
struct Uglier_t
{
};

// Exception gives no problems, as expected.
class my_problem : public std::exception
{
};

// Should be T_type.
template <typename type>
void type_template(type obj)
{
}

// Should be value.
template <int T_value>
int value_template()
{
    return T_value * 2;
}

} // namespace Gaia
