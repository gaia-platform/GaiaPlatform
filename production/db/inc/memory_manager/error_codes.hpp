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
    not_set = -1,

    success = 0,

    memory_address_not_aligned = 1,
    memory_offset_not_aligned = 2,
    memory_size_not_aligned = 3,

    memory_size_cannot_be_zero = 4,
    memory_size_too_large = 5,

    memory_address_out_of_range = 6,
    memory_offset_out_of_range = 7,

    insufficient_memory_size = 10,

    not_initialized = 100,
    already_initialized = 101,
    allocation_count_too_large = 102,
    invalid_argument_value = 103,
    operation_available_only_to_master_manager = 104,
};

}
}
}

