/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */    
namespace common
{

enum class error_code_t
{
    // common (0-0xff)
    not_set = -1, // do we need not_set? our code should never set this

    common_base = 0x0,
    success = common_base,
    not_initialized = common_base + 1,
    already_initialized = common_base + 2,
    invalid_argument_value = common_base + 3,

    // db (0x100 0x1ff)
    db_base = 0x100,
    memory_address_not_aligned = db_base,
    memory_offset_not_aligned = db_base + 1,
    memory_size_not_aligned = db_base + 2,
    memory_size_cannot_be_zero = db_base + 3,
    memory_size_too_large = db_base + 4,
    memory_address_out_of_range = db_base + 5,
    memory_offset_out_of_range = db_base + 6,
    insufficient_memory_size = db_base + 7,
    allocation_count_too_large = db_base + 8,
    operation_available_only_to_master_manager = db_base + 9,

    // rules (0x200 - 0x2ff)
    rule_base = 0x200,
    invalid_event_type = rule_base,
    invalid_rule_binding = rule_base + 1,
    duplicate_rule = rule_base + 2,
    rule_not_found = rule_base + 3,
    event_mode_not_supported = rule_base + 4,
};

/*@}*/
}
}

