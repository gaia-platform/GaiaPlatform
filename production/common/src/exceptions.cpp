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
