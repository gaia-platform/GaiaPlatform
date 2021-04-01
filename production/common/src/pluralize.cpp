/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/pluralize.hpp"

namespace gaia
{
namespace common
{

// TODO: Chuan will remove this.
std::string to_plural(std::string singular_string)
{
    int n = singular_string.size();
    singular_string.reserve(n + 2);
    char* string_content = singular_string.data();

    if (string_content[n - 1] == 'y') //ends with y
    {
        string_content[n - 1] = 'i';
        string_content[n] = 'e';
        string_content[n + 1] = 's';
        string_content[n + 2] = '\0';
    }
    if (string_content[n - 1] == 's' || ((string_content[n - 2] == 's') && (string_content[n - 1] == 'h'))) // ends with s or sh
    {
        string_content[n] = 'e';
        string_content[n + 1] = 's';
        string_content[n + 2] = '\0';
    }
    else //other cases
    {
        string_content[n] = 's';
        string_content[n + 1] = '\0';
    }

    return string_content;
}

} // namespace common
} // namespace gaia
