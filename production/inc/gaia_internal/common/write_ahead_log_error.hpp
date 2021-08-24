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
namespace persistence
{

/**
 * Thrown when any operation pertaining to the Write Ahead Log returns an error.
 */
class write_ahead_log_error : public gaia::common::gaia_exception
{
public:
    explicit write_ahead_log_error(const std::string& message, int code = 0)
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

} // namespace persistence
} // namespace db
} // namespace gaia
