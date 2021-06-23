/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>

#include <sstream>
#include <string>

#include "gaia/exception.hpp"

namespace gaia
{
namespace db
{

/**
 * Thrown when any calls to liburing API return an error.
 */
class io_uring_error : public gaia::common::gaia_exception
{
public:
    explicit io_uring_error(const std::string& message, int code = 0)
        : gaia_exception(message)
    {
        m_code = code;
    }

    int get_code()
    {
        return m_code;
    }

private:
    int m_code;
};

} // namespace db
} // namespace gaia
