/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "clang_tidy.h"

void Gaia::clang_tidy::Method()
{
    // wrong casing
    int PascalCase = 3;
}
void Gaia::clang_tidy::other_method(int BadArgument)
{
    // magic number bad
    int magic_number = BadArgument * 3.14;
}

