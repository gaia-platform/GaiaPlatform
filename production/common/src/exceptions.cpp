////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "gaia_internal/exceptions.hpp"

#include <cstring>

#include <sstream>
#include <string>

using namespace std;
using namespace gaia::common;

configuration_error_internal::configuration_error_internal(const char* filename)
{
    std::stringstream message;
    message << "Execution stopped because '" << filename << "' could not be found.";
    m_message = message.str();
}

logging::logger_exception_internal::logger_exception_internal()
{
    m_message = "Logger sub-system not initialized!";
}

optional_value_not_found_internal::optional_value_not_found_internal()
{
    m_message = "Optional has no value set!";
}
