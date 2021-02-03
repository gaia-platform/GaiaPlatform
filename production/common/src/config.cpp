/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/config.hpp"

#include "gaia/exceptions.hpp"

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

void error_if_not_exists(const char* filename)
{
    if (!file_exists(filename))
    {
        throw configuration_error(filename);
    }
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

} // namespace common
} // namespace gaia

// If the user passed in a filename, then throw an exception if it does not exist.
// If the user did not pass in a filename, then look under /opt/gaia/etc/ for the
// file and see if that exists.  If that file doesn't exist, then continue
// without throwing an exception.  Components can initialize themselves with
// appropriate defaults.
string gaia::common::get_conf_file_path(const char* user_file_path, const char* default_filename)
{
    string str;

    if (user_file_path)
    {
        error_if_not_exists(user_file_path);
        str = user_file_path;
    }
    else
    {
        str = get_default_conf_file_path(default_filename);
    }

    return str;
}
