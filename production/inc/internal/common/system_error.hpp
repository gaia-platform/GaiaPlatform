/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string.h>

#include <string>
#include <sstream>

#include "gaia_exception.hpp"

namespace gaia
{
namespace common
{

/**
 * Thrown when a system call returns an error.
 */
class system_error : public gaia_exception
{
public:
    system_error(const string& message)
        : gaia_exception(message)
    {
    }
};

inline void throw_system_error(const string& user_info, const int err=errno)
{
    std::stringstream ss;
    ss << user_info << " - " << (strerror(err));
    throw system_error(ss.str());
}

} // namespace common
} // namespace gaia
