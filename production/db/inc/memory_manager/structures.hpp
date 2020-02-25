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

// Encapsulates the set of variables needed for iterating over a memory list.
struct iteration_context
{
    memory_record* previous_record;
    auto_access_control auto_access_previous_record;

    memory_record* current_record;
    auto_access_control auto_access_current_record;

    iteration_context()
    {
        previous_record = nullptr;
        current_record = nullptr;
    }
};

// A collection of execution flags.
struct execution_flags
{
    // Requests execution status to be output to console.
    bool enable_console_output;

    // Requests additional validations to be performed during execution.
    bool enable_extra_validations;

    execution_flags()
    {
        enable_console_output = false;
        enable_extra_validations = false;
    }
};

}
}
}

