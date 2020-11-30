/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "memory_structures.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

// A collection of execution flags.
struct execution_flags_t
{
    // Requests execution status to be output to console.
    bool enable_console_output;

    execution_flags_t()
    {
        enable_console_output = false;
    }
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
