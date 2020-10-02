/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "clang_tidy.hpp"

#include <cstdlib>

#include <iostream>

// should use constexpr
#define PI 3.14

// should use using-declarations
using namespace gaia::common;
using namespace std;

// Avoid non-const global variable.
char a;

/**
 * Showcase how our .clang-tidy configuration works.
 */
void Gaia::clang_tidy::Method()
{
    // Should init variable.
    int a;

    // Wrong casing.
    int PascalCase = 3;

    cout << PascalCase << endl;
}
void Gaia::clang_tidy::other_method(int BadArgument)
{
    // Magic number bad.
    int magic_number = BadArgument * 3.14;
}

void narrowing_conversion()
{
    int i = 0;
    // Narrowing conversion from double to int.
    i += 0.1;
}

void no_malloc()
{
    // Do not use c-style cast to convert between unrelated types
    // use container or smart pointer.
    char* some_string = (char*)malloc(sizeof(char) * 20);
    free(some_string);
}

int no_c_style_casts()
{
    double var = 3.14;
    // Avoid c-style cast.
    return (int)var;
}

void pure_anarchy()
{
    // Next three statements will not produce warnings.
    int i = 42; // NOLINT

    // NOLINTNEXTLINE
    int j = 42;

    int y = 42; // NOLINT(cppcoreguidelines-avoid-magic-numbers)

    // Produces a warning, the actual check that fail is: cppcoreguidelines-avoid-magic-numbers.
    int k = 42; // NOLINT(google-runtime-operator)
}

int main(int argc, char* argv[])
{
    std::cout << "Hello World    " << PI << std::endl;
}
