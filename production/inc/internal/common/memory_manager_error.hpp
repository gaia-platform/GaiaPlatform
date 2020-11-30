/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string.h>

#include <sstream>
#include <string>

#include "gaia_exception.hpp"

namespace gaia
{
namespace db
{

/**
 * Thrown when any stack_allocator or memory_manager API returns an error.
 */
class memory_manager_error : public gaia::common::gaia_exception
{
public:
    memory_manager_error(const std::string& message, int code = 0)
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
