/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#ifndef PRODUCTION_CLANG_TIDY_HPP
#define PRODUCTION_CLANG_TIDY_HPP

#include "gaia_common.hpp"

#include <exception>

using namespace gaia::common;
using namespace std;

// should have c_ prefix
constexpr int constant_global = 2;

// should have c_ prefix
extern const int c_extern_constant = 6;

// should have g_ prefix
extern int global;

typedef long long int int_sixty_Four;

// should be lower case
namespace Gaia
{
// should have _t suffix. We can't catch that because of exceptions
class clang_tidy
{
    // Single-argument constructors must be marked explicit to avoid unintentional implicit conversions
    clang_tidy(int _member)
      : member(_member){};

    // wrong casing
    void Method();

    // argument wrong casing
    void other_method(int BadArgument);

    // should have implementation
    void no_implementation();

private:
    // should have m_ prefix
    int member;
};

// should have _t suffix
struct structure
{
};

// wrong casing
class Ugly_t
{
};

// wrong casing
struct Uglier_t
{
};

// exception gives no problems, as expected.
class my_problem : public std::exception
{
};

// should be T_type
template <typename type>
void type_template(type obj)
{
}

// should be value
template <int T_value>
int value_template()
{
    return T_value * 2;
}

} // namespace Gaia
#endif //PRODUCTION_CLANG_TIDY_HPP
