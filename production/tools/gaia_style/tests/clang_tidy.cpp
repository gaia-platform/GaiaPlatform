////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

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

// clang-tidy will flag all usage of const_cast regardless of whether
// they could lead to undefined behavior. This example will not lead to
// undefined behavior, anyway it could be avoided either way.
void no_const_cast()
{
    // reference
    int i = 3;
    const int& ri = i;

    const_cast<int&>(ri) = 4;

    // pointer
    int a = 0;
    const int* pa = &a;

    int* p = const_cast<int*>(pa);
    *p = 1;
}

// clang-tidy will flag all usage of const_cast regardless of whether
// they could lead to undefined behavior (UB). This example will lead to UB.
// clang-tidy will not treat it differently (eg. throwing an error instead
// of a warning), hence particular attention must be given to detect usage
// of const_cast that leads to UB.
// https://stackoverflow.com/a/7349716/1214125
void no_const_cast_undefined_behavior()
{
    // const reference
    const int i = 3;
    const int& ri = i;

    // Will produce a warning.
    const_cast<int&>(ri) = 4;

    // const pointer
    const int a = 0;
    const int* pa = &a;

    // Will produce a warning.
    int* p = const_cast<int*>(pa);
    *p = 1;
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
