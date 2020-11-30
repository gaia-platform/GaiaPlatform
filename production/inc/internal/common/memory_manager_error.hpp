/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string.h>

#include <sstream>
#include <string>

#include "gaia_exception.hpp"
#include "memory_types.hpp"

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
    memory_manager_error(const std::string& message, gaia::db::memory_manager::error_code_t code = gaia::db::memory_manager::error_code_t::success)
        : gaia_exception(message)
    {
        m_code = static_cast<std::underlying_type_t<gaia::db::memory_manager::error_code_t>>(code);
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
