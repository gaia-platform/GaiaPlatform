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
    MemoryRecord* pPreviousRecord;
    CAutoAccessControl autoAccessPreviousRecord;

    MemoryRecord* pCurrentRecord;
    CAutoAccessControl autoAccessCurrentRecord;

    IterationContext()
    {
        pPreviousRecord = nullptr;
        pCurrentRecord = nullptr;
    }
};

// A collection of execution flags.
struct ExecutionFlags
{
    // Requests execution status to be output to console.
    bool enableConsoleOutput;

    // Requests additional validations to be performed during execution.
    bool enableExtraValidations;

    ExecutionFlags()
    {
        enableConsoleOutput = false;
        enableExtraValidations = false;
    }
};

}
}
}

