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
namespace common
{

/**
 * Thrown when a system call returns an error.
 */
class system_error : public gaia_exception
{
public:
    explicit system_error(const std::string& message, int error_code = 0)
        : gaia_exception(message), m_error_code(error_code)
    {
    }

    int get_errno() const
    {
        return m_error_code;
    }

private:
    int m_error_code;
};

inline void throw_system_error(const std::string& user_info, int err = errno)
{
    std::stringstream ss;
    ss << user_info << " (System error code " << err << ": " << std::strerror(err) << ")";
    throw system_error(ss.str(), err);
}

} // namespace common
} // namespace gaia
