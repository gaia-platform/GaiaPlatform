/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "../inc/gaia_format.hpp"

#include "gaia_common.hpp"
#include "spdlog/spdlog.h"
#include "spdlog_setup/conf.h"

#include <algorithm>
#include <iostream>
#include <vector>

using std::abs;
using std::bind;
using std::cout;

namespace gaia
{
namespace style
{
class class_name_t
{
public:
    class_name_t(int private_member1, int private_member2, int private_member3)
      : m_private_member1(private_member1),
        m_private_member2(private_member2),
        m_private_member3(private_member3){};

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
    else
    {
        argument *= 3;
    }
    return argument * 2;
}

double class_name_t::method_with_many_arguments(
    int arg1,
    double arg2,
    double arg3,
    long long int arg4)
{
    return arg1 +
           arg2 +
           arg3 +
           arg4;
}

void short_statements()
{
    int a = 3;
    if (true)
    {
        a = 4;
    }

    while (true)
    {
        continue;
    }
}

void short_function()
{
    return;
}

void short_lambdas()
{
    auto l = [](int a) { a + 1; a + 2; };

    std::vector<int> vec = {1, 2, 3};

    std::sort(vec.begin(), vec.end(), [](int a, int b) { return a > b; });
}

std::string continue_indentation_width()
{
    std::string s("Something " +
                  std::string("plus" +
                              std::string("something else")));
    return s;
}

template <typename T>
T template_indentation(T argument)
{
    return argument + argument;
}

template <typename T>
void braced_style()
{
    std::vector<T> v{{}, {}, {}};
}

int space_after_c_cast()
{
    double var = 3.14;
    return (int) var;
}

bool space_after_not()
{
    bool yes = true;
    return !yes;
}

void pure_anarchy()
{
    // Note: some IDE such as Clion need an explicit configuration to support this tag
    // clang-format off
    int a1 =     3; int a2=2; if (true) { return;};
    // clang-format on
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
