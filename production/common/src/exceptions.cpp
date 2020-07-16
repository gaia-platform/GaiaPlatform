/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string.h>

#include <sstream>
#include <string>

#include <exceptions.hpp>

using namespace std;
using namespace gaia::common;

api_error::api_error(const char* api_name, int error_code)
{
    stringstream string_stream;
    string_stream << api_name << "() returned an unexpected error: "
        << strerror(error_code) << " (" <<  error_code << ").";
    m_message = string_stream.str();
}
