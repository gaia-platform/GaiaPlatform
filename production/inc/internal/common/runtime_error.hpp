/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// C headers
#include <string.h>
// C++ headers
#include <string>
#include <sstream>

namespace gaia
{
namespace common
{

inline void throw_runtime_error(const std::string& info, const int err=errno)
{
    std::stringstream ss;
    ss << info << " - " << (strerror(err));
    throw std::runtime_error(ss.str());
}

} // namespace common
} // namespace gaia
