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
    ifstream the_file(filename);
    return static_cast<bool>(the_file);
}

void error_if_not_exists(const char* filename)
{
    if (!file_exists(filename))
    {
        throw configuration_error(filename);
    }
}

string get_default_conf_file(const char* default_filename)
{
    // Default locations for log files are placed under/opt/gaia/etc/
    static const char* c_default_conf_directory = "/opt/gaia/etc/";
    string str = c_default_conf_directory;
    str.append(default_filename);

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
// file and see if that exists.  If that file doesn't exist, then continue on
// without throwing an exception.  Components can initialize themselves with
// appropriate defaults.
string gaia::common::get_conf_file(const char* user_filename, const char* default_filename)
{
    string str;

    if (user_filename)
    {
        error_if_not_exists(user_filename);
        str = user_filename;
    }
    else
    {
        str = get_default_conf_file(default_filename);
    }

    return str;
}
