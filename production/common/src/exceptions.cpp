/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/exceptions.hpp"

#include <cstring>

#include <sstream>
#include <string>

using namespace std;
using namespace gaia::common;

api_error::api_error(const char* api_name, int error_code)
{
    stringstream string_stream;
    string_stream << api_name << "() returned an unexpected error: ";
    string_stream << strerror(error_code) << " (" << error_code << ").";
    m_message = string_stream.str();
}

configuration_error::configuration_error(const char* key, int value)
{
    std::stringstream message;
    message << "Invalid value '" << value << "' provided for setting '" << key << "'.";
    m_message = message.str();
}

configuration_error::configuration_error(const char* filename)
{
    std::stringstream message;
    message << "Execution stopped because '" << filename << "' could not be found.";
    m_message = message.str();
}
