/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#ifndef PRODUCTION_CLANG_TIDY_H
#define PRODUCTION_CLANG_TIDY_H

#include <exception>
#include "gaia_common.hpp"

using namespace gaia::common;

// should have c_ prefix
constexpr int constant_global = 2;

// should have c_ prefix
extern const int extern_constant = 6;

// should have g_ prefix
extern int global;


typedef long long int int_sixty_Four;

// should be lower case
namespace Gaia
{

// should have _t suffix. We can't catch that because of exceptions
class clang_tidy
{
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
class my_problem : public std::exception {

};

} // namespace Gaia
#endif //PRODUCTION_CLANG_TIDY_H
