/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <sstream>

#include "gaia_internal/common/retail_assert.hpp"

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
    base_memory_manager_t() = default;

    // Basic accessors.
    inline uint8_t* get_base_memory_address() const;
    inline address_offset_t get_start_memory_offset() const;
    inline size_t get_total_memory_size() const;

    // Sets stack_allocator_t execution flags.
    inline void set_execution_flags(const execution_flags_t& execution_flags);

    // Helper function for allocation alignment.
    // Allocation sizes need to be rounded up to the closest 64B multiple.
    inline static size_t calculate_allocation_size(size_t requested_size);

protected:
    // Sanity checks.
    static inline void validate_offset_alignment(address_offset_t memory_offset);

    inline void validate_managed_memory_range() const;

    inline void validate_address(const uint8_t* const memory_address) const;
    inline void validate_offset(address_offset_t memory_offset) const;
    inline void validate_slot_offset(slot_offset_t slot_offset) const;
    inline void validate_size(size_t memory_size) const;

    // Gets the offset corresponding to a memory address.
    inline address_offset_t get_offset(const uint8_t* const memory_address) const;

    // Gets the memory address corresponding to an offset.
    inline uint8_t* get_address(address_offset_t memory_offset) const;

    // Gets the chunk address offset corresponding to an offset.
    inline address_offset_t get_chunk_address_offset(address_offset_t memory_offset) const;

    // Gets the chunk offset corresponding to an offset.
    inline chunk_offset_t get_chunk_offset(address_offset_t memory_offset) const;

    // Gets the slot offset within a chunk corresponding to an offset.
    inline slot_offset_t get_slot_offset(address_offset_t memory_offset) const;

protected:
    // The base memory address relative to which we compute our offsets.
    uint8_t* m_base_memory_address{nullptr};

    // The memory offset at which our buffer starts (in case we only own a window into a larger memory block).
    address_offset_t m_start_memory_offset{c_invalid_address_offset};

    // The total size of the memory segment in which we operate.
    size_t m_total_memory_size{0};

    // Our execution flags.
    execution_flags_t m_execution_flags{};
};

#include "base_memory_manager.inc"

} // namespace memory_manager
} // namespace db
} // namespace gaia
