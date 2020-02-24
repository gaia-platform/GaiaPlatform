/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia
{
namespace db
{
namespace memory_manager
{

enum EMemoryManagerErrorCode
{
    mmec_NotSet = -1,
    mmec_Success = 0,

    mmec_MemoryAddressNotAligned = 1,
    mmec_MemoryOffsetNotAligned = 2,
    mmec_MemorySizeNotAligned = 3,

    mmec_MemorySizeCannotBeZero = 4,
    mmec_MemorySizeTooLarge = 5,

    mmec_MemoryAddressOutOfRange = 6,
    mmec_MemoryOffsetOutOfRange = 7,

    mmec_InsufficientMemorySize = 10,

    mmec_NotInitialized = 100,
    mmec_AlreadyInitialized = 101,
    mmec_AllocationCountTooLarge = 102,
    mmec_InvalidArgumentValue = 103,
    mmec_OperationAvailableOnlyToMasterManager = 104,
};

}
}
}

