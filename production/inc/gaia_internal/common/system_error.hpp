/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>

#include <sstream>
#include <string>

#include "gaia/exceptions.hpp"

namespace gaia
{
namespace common
{

inline void throw_system_error(const std::string& user_info, int err = errno)
{
    std::stringstream ss;
    ss << user_info << " - " << (::strerror(err));
    throw system_error(ss.str(), err);
}

} // namespace common
} // namespace gaia
