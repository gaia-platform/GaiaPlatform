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

// Avoid non-const global variable
char a;

void Gaia::clang_tidy::Method()
{
    // should init variable
    int a;

    // wrong casing
    int PascalCase = 3;

    cout << PascalCase << endl;
}
void Gaia::clang_tidy::other_method(int BadArgument)
{
    // magic number bad
    int magic_number = BadArgument * 3.14;
}

void narrowing_conversion() {
    int i = 0;
    // narrowing conversion from double to int
    i += 0.1;
}

void no_malloc() {
    // use container or smart pointer
    char* some_string = (char*) malloc(sizeof(char) * 20);
    free(some_string);
}


void pure_anarchy() {
    // Next three statements will not produce warnings
    int i = 42; // NOLINT

    // NOLINTNEXTLINE
    int j = 42;

    int y = 42; // NOLINT(cppcoreguidelines-avoid-magic-numbers)

    // produces a warning, the actual check that fail is: cppcoreguidelines-avoid-magic-numbers
    int k = 42; // NOLINT(google-runtime-operator)
}


int main(int argc, char* argv[])
{
    std::cout << "Hello World    " << PI << std::endl;
}
