/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "clang_tidy.hpp"

// deprecated
#include <stdlib.h>

#include <iostream>
#include <map>

// should use constexpr
#define PI 3.14

// should use using-declarations
using namespace gaia::common;
using namespace std;
using std::map; // good

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
}

void magic_numbers(int arg)
{

    // Complain 3.14 is a magic number.
    int magic_number = arg * 3.14; // NOLINT

    // does not complain about 0/2/1
    for (int i = 0; i < arg / 2; i = i + 1)
    {
    }
}

void narrowing_conversion()
{
    int i = 0;
    // Narrowing conversion from double to int.
    i += 0.1;
}

template <typename T1, typename T2>
map<int, int>* thingy()
{
    return nullptr;
}

void use_auto()
{
    // should use auto since the type is explicit on the right side.
    map<int, int>* m = new map<int, int>();

    // should not use auto because we don't know the type of "thingy"
    map<int, int>* m2 = thingy<int, int>();
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

    int YYY = 42; // NOLINT(cppcoreguidelines-avoid-magic-numbers)

    // Produces a warning, the actual check that fail is: cppcoreguidelines-avoid-magic-numbers.
    int k = 42; // NOLINT(google-runtime-operator)
}

int main(int argc, char* argv[])
{
    std::cout << "Hello World    " << PI << std::endl;
}
