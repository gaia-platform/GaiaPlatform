/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <stdexcept>

#include "gaia/gaia_exception.hpp"
#include <libexplain/close.h>

#include "system_error.hpp"

namespace gaia
{
namespace common
{

inline void close_fd(int& fd)
{
    int tmp = fd;
    fd = -1;
    if (-1 == ::close(tmp))
    {
        int err = errno;
        const char* reason = ::explain_close(tmp);
        throw system_error(reason, err);
    }
}

} // namespace common
} // namespace gaia
