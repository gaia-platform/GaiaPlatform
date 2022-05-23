////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "gaia_internal/common/config.hpp"

#include <cstring>

using namespace std;
namespace gaia
{
namespace common
{

bool file_exists(const char* filename)
{
    ifstream file(filename);
    return static_cast<bool>(file);
}

string get_default_conf_file_path(const char* default_filename)
{
    // Default locations for log files are placed under /opt/gaia/etc/
    filesystem::path str = c_default_conf_directory;
    str /= default_filename;

    if (!file_exists(str.c_str()))
    {
        // Okay, just return an empty string then.
        str.clear();
    }

    return str;
}

// If the user passed in a file path, make sure it exists.
// If the user did not pass in a file path, then look under /opt/gaia/etc/ for the
// file and see if that exists.  If that a file doesn't exist, return an empty string.
string get_conf_file_path(const char* user_file_path, const char* default_filename)
{
    string str;

    if (user_file_path && strlen(user_file_path) > 0)
    {
        if (file_exists(user_file_path))
        {
            str = user_file_path;
        }
    }
    else
    {
        str = get_default_conf_file_path(default_filename);
    }

    return str;
}

} // namespace common
} // namespace gaia
