/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "memory_types.hpp"
#include "structures.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

constexpr char c_debug_output_separator_line_start[] = ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
constexpr char c_debug_output_separator_line_end[] = "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";

// This class provides operations relevant to the management of a memory range
// defined by a base memory address, start offset, and total size.
class base_memory_manager_t
{
public:
    base_memory_manager_t();

    // Basic accessors.
    uint8_t* get_base_memory_address() const;
    address_offset_t get_start_memory_offset() const;
    size_t get_total_memory_size() const;

    // Sets stack_allocator_t execution flags.
    void set_execution_flags(const execution_flags_t& execution_flags);

    // Helper function for allocation alignment.
    static size_t calculate_allocation_size(size_t requested_size);

    // Sanity checks.
    static void validate_address_alignment(const uint8_t* const memory_address);
    static void validate_offset_alignment(address_offset_t memory_offset);
    static void validate_size_alignment(size_t memory_size);

    void validate_managed_memory_range() const;

    void validate_address(const uint8_t* const memory_address) const;
    void validate_offset(address_offset_t memory_offset) const;
    void validate_size(size_t memory_size) const;

    // Gets the offset corresponding to a memory address.
    address_offset_t get_offset(const uint8_t* const memory_address) const;

    // Gets the memory address corresponding to an offset.
    uint8_t* get_address(address_offset_t memory_offset) const;

protected:
    // The base memory address relative to which we compute our offsets.
    uint8_t* m_base_memory_address;

    // The memory offset at which our buffer starts (in case we only own a window into a larger memory block).
    address_offset_t m_start_memory_offset;

    // The total size of the memory segment in which we operate.
    size_t m_total_memory_size;

    // Our execution flags.
    execution_flags_t m_execution_flags;
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
