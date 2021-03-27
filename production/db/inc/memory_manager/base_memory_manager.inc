/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

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

size_t base_memory_manager_t::calculate_allocation_size(size_t requested_size)
{
    common::retail_assert(requested_size > 0, "Requested allocation size is 0!");

    size_t allocation_size = ((requested_size + c_slot_size - 1) / c_slot_size) * c_slot_size;

    return allocation_size;
}

void base_memory_manager_t::validate_address_alignment(const uint8_t* const memory_address)
{
    auto memory_address_as_integer = reinterpret_cast<size_t>(memory_address);
    common::retail_assert(
        memory_address_as_integer % c_allocation_alignment == 0,
        "Misaligned memory address!");
}

void base_memory_manager_t::validate_offset_alignment(address_offset_t memory_offset)
{
    common::retail_assert(
        memory_offset % c_slot_size == 0,
        "Misaligned memory offset!");
}

void base_memory_manager_t::validate_size_alignment(size_t memory_size)
{
    common::retail_assert(
        memory_size % c_slot_size == 0,
        "Misaligned memory size!");
}

void base_memory_manager_t::validate_managed_memory_range() const
{
    std::stringstream message1;
    message1
        << "Total memory size (" << m_total_memory_size
        << ") is too large for start memory offset (" << m_start_memory_offset
        << ")!";
    common::retail_assert(
        m_start_memory_offset + m_total_memory_size > m_start_memory_offset,
        message1.str());

    auto base_memory_address_as_integer = reinterpret_cast<size_t>(m_base_memory_address);
    std::stringstream message2;
    message2
        << "Start memory offset (" << m_start_memory_offset
        << ") is too large for base memory address (" << base_memory_address_as_integer
        << ")!";
    common::retail_assert(
        base_memory_address_as_integer + m_start_memory_offset >= base_memory_address_as_integer,
        message2.str());

    size_t start_memory_address_as_integer = base_memory_address_as_integer + m_start_memory_offset;
    std::stringstream message3;
    message3
        << "Total memory size (" << m_total_memory_size
        << ") is too large for start memory address (" << start_memory_address_as_integer
        << ")!";
    common::retail_assert(
        start_memory_address_as_integer + m_total_memory_size > start_memory_address_as_integer,
        message3.str());
}

void base_memory_manager_t::validate_address(const uint8_t* const memory_address) const
{
    validate_address_alignment(memory_address);

    common::retail_assert(
        memory_address >= m_base_memory_address + m_start_memory_offset
            && memory_address <= m_base_memory_address + m_start_memory_offset + m_total_memory_size,
        "Memory address is outside managed memory range!");
}

void base_memory_manager_t::validate_offset(address_offset_t memory_offset) const
{
    common::retail_assert(
        memory_offset != c_invalid_address_offset,
        "Memory offset is invalid!");

    validate_offset_alignment(memory_offset);

    common::retail_assert(
        memory_offset >= m_start_memory_offset
            && memory_offset <= m_start_memory_offset + m_total_memory_size,
        "Memory offset is outside managed memory range!");
}

void base_memory_manager_t::validate_size(size_t memory_size) const
{
    validate_size_alignment(memory_size);

    common::retail_assert(
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
