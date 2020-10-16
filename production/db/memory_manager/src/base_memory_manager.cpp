/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "base_memory_manager.hpp"

#include <unistd.h>

#include "retail_assert.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;

base_memory_manager_t::base_memory_manager_t()
{
    m_base_memory_address = nullptr;
    m_base_memory_offset = c_invalid_offset;
    m_total_memory_size = 0;
}

uint8_t* base_memory_manager_t::get_base_memory_address() const
{
    return m_base_memory_address;
}

address_offset_t base_memory_manager_t::get_base_memory_offset() const
{
    return m_base_memory_offset;
}

size_t base_memory_manager_t::get_total_memory_size() const
{
    return m_total_memory_size;
}

void base_memory_manager_t::set_execution_flags(const execution_flags_t& execution_flags)
{
    m_execution_flags = execution_flags;
}

bool base_memory_manager_t::validate_address_alignment(const uint8_t* const memory_address) const
{
    auto memory_address_as_integer = reinterpret_cast<size_t>(memory_address);
    return (memory_address_as_integer % c_memory_alignment == 0);
}

bool base_memory_manager_t::validate_offset_alignment(address_offset_t memory_offset) const
{
    return (memory_offset % c_memory_alignment == 0);
}

bool base_memory_manager_t::validate_size_alignment(size_t memory_size) const
{
    return (memory_size % c_memory_alignment == 0);
}

error_code_t base_memory_manager_t::validate_address(const uint8_t* const memory_address) const
{
    if (!validate_address_alignment(memory_address))
    {
        return error_code_t::memory_address_not_aligned;
    }

    if (memory_address < m_base_memory_address + m_base_memory_offset
        || memory_address > m_base_memory_address + m_base_memory_offset + m_total_memory_size)
    {
        return error_code_t::memory_address_out_of_range;
    }

    return error_code_t::success;
}

error_code_t base_memory_manager_t::validate_offset(address_offset_t memory_offset) const
{
    if (memory_offset == c_invalid_offset)
    {
        return error_code_t::invalid_memory_offset;
    }

    if (!validate_offset_alignment(memory_offset))
    {
        return error_code_t::memory_offset_not_aligned;
    }

    if (memory_offset < m_base_memory_offset
        || memory_offset > m_base_memory_offset + m_total_memory_size)
    {
        return error_code_t::memory_offset_out_of_range;
    }

    return error_code_t::success;
}

error_code_t base_memory_manager_t::validate_size(size_t memory_size) const
{
    if (!validate_size_alignment(memory_size))
    {
        return error_code_t::memory_size_not_aligned;
    }

    if (memory_size == 0)
    {
        return error_code_t::memory_size_cannot_be_zero;
    }

    if (memory_size > m_total_memory_size)
    {
        return error_code_t::memory_size_too_large;
    }

    return error_code_t::success;
}

address_offset_t base_memory_manager_t::get_offset(const uint8_t* const memory_address) const
{
    retail_assert(
        validate_address(memory_address) == error_code_t::success,
        "get_offset() was called with an invalid address!");

    size_t memory_offset = memory_address - m_base_memory_address;

    return memory_offset;
}

uint8_t* base_memory_manager_t::get_address(address_offset_t memory_offset) const
{
    retail_assert(
        validate_offset(memory_offset) == error_code_t::success,
        "get_address() was called with an invalid offset!");

    uint8_t* memory_address = m_base_memory_address + memory_offset;

    return memory_address;
}

memory_allocation_metadata_t* base_memory_manager_t::read_allocation_metadata(address_offset_t memory_offset) const
{
    retail_assert(
        validate_offset(memory_offset) == error_code_t::success,
        "read_allocation_metadata() was called with an invalid offset!");

    retail_assert(
        memory_offset >= sizeof(memory_allocation_metadata_t),
        "read_allocation_metadata() was called with an offset that is too small!");

    address_offset_t allocation_metadata_offset = memory_offset - sizeof(memory_allocation_metadata_t);
    uint8_t* allocation_metadata_address = get_address(allocation_metadata_offset);
    auto allocation_metadata = reinterpret_cast<memory_allocation_metadata_t*>(allocation_metadata_address);

    return allocation_metadata;
}
