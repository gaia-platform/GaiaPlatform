/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "gaia/exception.hpp"

namespace gaia
{
namespace db
{

/**
 * Thrown when any stack_allocator or memory_manager API returns an error.
 */
class memory_allocation_error : public gaia::common::gaia_exception
{
public:
    explicit memory_allocation_error(const std::string& message)
        : gaia_exception(message)
    {
    }
};

} // namespace db
} // namespace gaia
