/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstring>

#include <exceptions.hpp>
#include <sstream>
#include <string>

using namespace std;
using namespace gaia::common;

api_error::api_error(const char* api_name, int error_code)
{
    stringstream string_stream;
    string_stream << api_name << "() returned an unexpected error: "
                  << strerror(error_code) << " (" << error_code << ").";
    m_message = string_stream.str();
}

configuration_error::configuration_error(const char* key, int value)
{
    std::stringstream message;
    message << "Invalid value '" << value << "' provided for setting '" << key << "'.";
    m_message = message.str();
}
