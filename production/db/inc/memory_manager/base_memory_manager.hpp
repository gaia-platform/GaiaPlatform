/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "types.hpp"
#include "error_codes.hpp"
#include "structures.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

const char* const c_debug_output_separator_line_start = ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
const char* const c_debug_output_separator_line_end = "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";

class base_memory_manager_t
{
public:

    static const size_t c_memory_alignment = sizeof(uint64_t);

public:

    base_memory_manager_t();

    // Sets stack_allocator_t execution flags.
    void set_execution_flags(const execution_flags_t& execution_flags);

    // Sanity checks.
    bool validate_address_alignment(const uint8_t* const memory_address) const;
    bool validate_offset_alignment(address_offset_t memory_offset) const;
    bool validate_size_alignment(size_t memory_size) const;

    error_code_t validate_address(const uint8_t* const memory_address) const;
    error_code_t validate_offset(address_offset_t memory_offset) const;
    error_code_t validate_size(size_t memory_size) const;

    // Gets the offset corresponding to a memory address.
    address_offset_t get_offset(const uint8_t* const memory_address) const;

    // Gets the memory address corresponding to an offset.
    uint8_t* get_address(address_offset_t memory_offset) const;

    // Gets the allocation metadata record given the base address of the allocation.
    memory_allocation_metadata_t* read_allocation_metadata(address_offset_t memory_offset) const;

protected:

    // The base memory address relative to which we compute our offsets.
    uint8_t* m_base_memory_address;

    // The base memory offset at which our buffer starts (in case we only own a window into a larger memory block).
    address_offset_t m_base_memory_offset;

    // The total size of the memory segment in which we operate.
    size_t m_total_memory_size;

    // Our execution flags.
    execution_flags_t m_execution_flags;
};

}
}
}
