////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <regex>
#include <sstream>
#include <string>

#include "gaia/exception.hpp"

inline constexpr char c_command_prefix = '\\';

/**
 * Thrown when the given command is invalid
 */
class invalid_command : public gaia::common::gaia_exception
{
public:
    explicit invalid_command(const std::string& cmd)
    {
        std::stringstream message;
        message << "Invalid command: '" << cmd << "'.";
        m_message = message.str();
    }

    invalid_command(const std::string& cmd, const std::regex_error& error)
    {
        std::stringstream message;
        message
            << "Invalid command: '" << cmd << "'. "
            << "Regular expression error '" << error.code() << "': '" << error.what() << "'.";
        m_message = message.str();
    }
};

/**
 * Handle meta commands.
 *
 * @return whether the execution should continue,
 * i.e. false is returned when the quit command is entered.
 */
bool handle_meta_command(const std::string& line);
