/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <regex>
#include <string>
#include <sstream>

#include "gaia_exception.hpp"

inline constexpr char c_command_prefix = '\\';

/**
 * Thrown when the given command is invalid
 */
class invalid_command : public gaia::common::gaia_exception {
public:
    invalid_command(const string& cmd) {
        stringstream message;
        message << "Invalid command: " << cmd << ".";
        m_message = message.str();
    }

    invalid_command(const string& cmd, const regex_error& error) {
        stringstream message;
        message
            << "Invalid command: " << cmd << ". "
            << "Regular expression error " << error.code() << ": " << error.what() << ".";
        m_message = message.str();
    }
};

void handle_slash_command(const string& line);
