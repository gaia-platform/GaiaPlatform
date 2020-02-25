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
struct IterationContext
{
    MemoryRecord* previous_record;
    CAutoAccessControl auto_access_previous_record;

    MemoryRecord* current_record;
    CAutoAccessControl auto_access_current_record;

    IterationContext()
    {
        previous_record = nullptr;
        current_record = nullptr;
    }
};

// A collection of execution flags.
struct ExecutionFlags
{
    // Requests execution status to be output to console.
    bool enable_console_output;

    // Requests additional validations to be performed during execution.
    bool enable_extra_validations;

    ExecutionFlags()
    {
        enable_console_output = false;
        enable_extra_validations = false;
    }
};

}
}
}

