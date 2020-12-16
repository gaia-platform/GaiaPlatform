/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "base_memory_manager.hpp"

#include <unistd.h>

#include <sstream>

#include "retail_assert.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;

base_memory_manager_t::base_memory_manager_t()
{
    m_base_memory_address = nullptr;
    m_start_memory_offset = c_invalid_offset;
    m_total_memory_size = 0;
}

uint8_t* base_memory_manager_t::get_base_memory_address() const
{
    return m_base_memory_address;
}

address_offset_t base_memory_manager_t::get_start_memory_offset() const
{
    return m_start_memory_offset;
}

size_t base_memory_manager_t::get_total_memory_size() const
{
    return m_total_memory_size;
}

void base_memory_manager_t::set_execution_flags(const execution_flags_t& execution_flags)
{
    m_execution_flags = execution_flags;
}

// Allocations need to be made on a 64B boundary and need to leave space
// for a metadata header before the next 64B boundary.
//
// We thus need to update the requested size to a value such that
// when computing the modulo for our alignment,
// it would result in a minimum allocation size.
size_t base_memory_manager_t::calculate_allocation_size(size_t requested_size)
{
    if (requested_size < c_minimum_allocation_size)
    {
        return c_minimum_allocation_size;
    }

    size_t allocation_size = 0;
    size_t extra_block_size = requested_size % c_allocation_alignment;

    if (extra_block_size <= c_minimum_allocation_size)
    {
        // Extra fits within the minimum allocation size, so replace it with that instead.
        allocation_size = requested_size - extra_block_size + c_minimum_allocation_size;
    }
    else
    {
        // Extra exceeds the minimum allocation, so replace it with that plus an extra 64B block.
        allocation_size = requested_size - extra_block_size + c_allocation_alignment + c_minimum_allocation_size;
    }

    // Handle the extreme case in which the requested size is so large
    // that we'd get an integer overflow in the preceding calculations.
    return (allocation_size < requested_size) ? 0 : allocation_size;
}

// Raw allocations need to be offset by 56B from 64B boundaries
// and need to end the same way, which means that they should be adjusted
// to the nearest higher alignment multiplier.
size_t base_memory_manager_t::calculate_raw_allocation_size(size_t requested_size)
{
    if (requested_size < c_allocation_alignment)
    {
        return c_allocation_alignment;
    }

    size_t allocation_size = requested_size;
    size_t extra_block_size = requested_size % c_allocation_alignment;

    if (extra_block_size > 0)
    {
        // Bump the size to the next multiple of the allocation alignment.
        allocation_size = requested_size - extra_block_size + c_allocation_alignment;
    }

    // Handle the extreme case in which the requested size is so large
    // that we'd get an integer overflow in the preceding calculations.
    return (allocation_size < requested_size) ? 0 : allocation_size;
}

void base_memory_manager_t::validate_address_alignment(const uint8_t* const memory_address)
{
    auto memory_address_as_integer = reinterpret_cast<size_t>(memory_address);
    retail_assert(
        memory_address_as_integer % c_memory_alignment == 0,
        "Misaligned memory address!");
}

void base_memory_manager_t::validate_offset_alignment(address_offset_t memory_offset)
{
    retail_assert(
        memory_offset % c_memory_alignment == 0,
        "Misaligned memory offset!");
}

void base_memory_manager_t::validate_size_alignment(size_t memory_size)
{
    retail_assert(
        memory_size % c_memory_alignment == 0,
        "Misaligned memory size!");
}

void base_memory_manager_t::validate_managed_memory_range() const
{
    std::stringstream message1;
    message1
        << "Total memory size (" << m_total_memory_size
        << ") is too large for start memory offset (" << m_start_memory_offset
        << ")!";
    retail_assert(
        m_start_memory_offset + m_total_memory_size > m_start_memory_offset,
        message1.str());

    auto base_memory_address_as_integer = reinterpret_cast<size_t>(m_base_memory_address);
    std::stringstream message2;
    message2
        << "Start memory offset (" << m_start_memory_offset
        << ") is too large for base memory address (" << base_memory_address_as_integer
        << ")!";
    retail_assert(
        base_memory_address_as_integer + m_start_memory_offset >= base_memory_address_as_integer,
        message2.str());

    size_t start_memory_address_as_integer = base_memory_address_as_integer + m_start_memory_offset;
    std::stringstream message3;
    message3
        << "Total memory size (" << m_total_memory_size
        << ") is too large for start memory address (" << start_memory_address_as_integer
        << ")!";
    retail_assert(
        start_memory_address_as_integer + m_total_memory_size > start_memory_address_as_integer,
        message3.str());
}

void base_memory_manager_t::validate_address(const uint8_t* const memory_address) const
{
    validate_address_alignment(memory_address);

    retail_assert(
        memory_address >= m_base_memory_address + m_start_memory_offset
            && memory_address <= m_base_memory_address + m_start_memory_offset + m_total_memory_size,
        "Memory address is outside managed memory range!");
}

void base_memory_manager_t::validate_offset(address_offset_t memory_offset) const
{
    retail_assert(
        memory_offset != c_invalid_offset,
        "Memory offset is invalid!");

    validate_offset_alignment(memory_offset);

    retail_assert(
        memory_offset >= m_start_memory_offset
            && memory_offset <= m_start_memory_offset + m_total_memory_size,
        "Memory offset is outside managed memory range!");
}

void base_memory_manager_t::validate_size(size_t memory_size) const
{
    validate_size_alignment(memory_size);

    retail_assert(
        memory_size > 0,
        "Memory size should not be 0!");
}

address_offset_t base_memory_manager_t::get_offset(const uint8_t* const memory_address) const
{
    validate_address(memory_address);

    size_t memory_offset = memory_address - m_base_memory_address;

    return memory_offset;
}

uint8_t* base_memory_manager_t::get_address(address_offset_t memory_offset) const
{
    validate_offset(memory_offset);

    uint8_t* memory_address = m_base_memory_address + memory_offset;

    return memory_address;
}

memory_allocation_metadata_t* base_memory_manager_t::read_allocation_metadata(address_offset_t memory_offset) const
{
    validate_offset(memory_offset);

    retail_assert(
        memory_offset >= sizeof(memory_allocation_metadata_t),
        "read_allocation_metadata() was called with an offset that is too small!");

    address_offset_t allocation_metadata_offset = memory_offset - sizeof(memory_allocation_metadata_t);
    uint8_t* allocation_metadata_address = get_address(allocation_metadata_offset);
    auto allocation_metadata = reinterpret_cast<memory_allocation_metadata_t*>(allocation_metadata_address);

    return allocation_metadata;
}
