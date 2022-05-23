////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "clang_format.hpp"

#include <cpio.h>
#include <stdlib.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include <flatbuffers/base.h>

#include "gaia/common.hpp"
#include "gaia/component.hpp"
#include "gaia/db/db.hpp"

// Does not move include with comments.
#include <iterator>
#include <list>
#include <string>

#include "gaia/exception.hpp"

#ifdef VAR
#include <filesystem>
#include <functional>

#include "gaia_internal/catalog/gaia_catalog.h"
#endif

using std::abs;
using std::bind;
using std::cout;

/**
 * Showcase how our .clang-format configuration works.
 */
namespace gaia
{

class inner_namespace_t
{
};

namespace style
{
class class_name_t
{

public:
    class_name_t(int private_member1, int private_member2, int private_member3)
        : m_private_member1(private_member1)
        , m_private_member2(private_member2)
        , m_private_member3(private_member3){};

    int method_name(int argument);

    double method_with_many_arguments(int arg1, double arg2, double arg3, long long int arg4);

private:
    int m_private_member1;
    int m_private_member2;
    int m_private_member3;
};

int class_name_t::method_name(int argument)
{
    if (argument == 3)
    {
        argument *= 2;
    }
    else if (argument < 4)
    {
        argument *= 3;
    }
    else
    {
        argument *= 0;
    }
    return argument * 2;
}

// the arguments are either all on the same line or all on a new line.
double class_name_t::method_with_many_arguments(
    int arg1,
    double arg2,
    double arg3,
    long long int arg4)
{
    // breaks before binary operator
    return arg1
        + arg2
        + arg3 + arg4; // NOLINT
}

void short_statements()
{
    int a = 3;
    // NOLINTNEXTLINE
    if (true)
    {
        a = 4;
    }

    while (a-- > 0)
    {
        continue;
    }
}

void short_function(int a)
{
    return;
}

void short_lambdas()
{
    auto l = [](int a) {
        a + 1;
        a + 2;
    };

    std::vector<int> vec = {1, 2, 3};

    std::sort(vec.begin(), vec.end(), [](int a, int b) { return a > b; });
}

std::string continue_indentation_width()
{
    std::string variable_with_rather_weird_name(
        "Something "
        + std::string(
            "plus"
            + std::string("something else")));

    std::string variable_with_rather_weird_name2("Something " + std::string("plus" + std::string("something else")));

    short_function(
        3 + 4);

    return variable_with_rather_weird_name;
}

template <typename T_type>
T_type template_indentation(T_type argument)
{
    return argument + argument;
}

void braced_style()
{
    std::vector<int> v{{}, {}, {}};
}

int no_space_after_c_cast()
{
    double var = 3.14; // NOLINT
    return (int)var; // NOLINT
}

bool space_after_not()
{
    bool yes = true;
    return !yes;
}

int binary_operator()
{
    int var1 = 10;
    int var2 = 30;
    int var3 = 50;

    int result1 = var1 + var2 + var3;

    int result2
        = var1 + var2 + var3;

    int result3 = var1
        + var2 + var3;

    return result1
        + result2
        + result3;
}

int ternary_operator()
{
    int var1 = 10;
    int var2 = 30;

    int result1 = (var1 > var2) ? var1 : var2;

    int result2 = (var1 > var2) ? var1
                                : var2;

    int result3 = (var1 > var2)
        ? var1
        : var2;

    int result4
        = (var1 > var2)
        ? var1
        : var2;

    return result1 < result2
        ? result3
        : result4;
}

void pure_anarchy()
{
    // Note: some IDE such as Clion need an explicit configuration to support this tag
    // clang-format off
    int a1 =     3; int a2=2; if (true) { return;}; //NOLINT
    // clang-format on
}

void const_position()
{
    // The current version of clang-format only supports west-const positioning.
    // Read this about the east/west const debate:
    // https://mariusbancila.ro/blog/2018/11/23/join-the-east-const-revolution/
    // https://reviews.llvm.org/D69764

    int* p1 = nullptr; // p1 is a mutable pointer to a mutable int
    int const* p2 = nullptr; // p2 is a mutable pointer to a constant int
    int* const p3 = nullptr; // p3 is a constant pointer to a mutable int
    const int* const p4 = nullptr; // p4 is a constant pointer to a constant int
}

void chained_calls()
{
    // Unfortunately clang-format handle chained calls in a pretty ugly way.

    // Option number 1
    auto str = std::string("something")
                   .append("1")
                   .append("2")
                   .append("3")
                   .append("4")
                   .append("5")
                   .append("6");

    // Option number 2
    auto str2
        = std::string("something")
              .append("1")
              .append("2")
              .append("3")
              .append("4")
              .append("5")
              .append("6");
}

class base_t
{
};

class derived_t : base_t
{
};

} // namespace style
} // namespace gaia

int main(int argc, char* argv[])
{
    std::cout << "Hello World    " << std::endl;
}
