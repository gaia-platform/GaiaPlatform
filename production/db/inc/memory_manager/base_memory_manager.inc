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
    ASSERT_PRECONDITION(requested_size > 0, "Requested allocation size is 0!");

    size_t allocation_size = ((requested_size + c_slot_size_bytes - 1) / c_slot_size_bytes) * c_slot_size_bytes;

    ASSERT_POSTCONDITION(
        allocation_size > 0,
        "Allocation size should not be 0!");

    ASSERT_POSTCONDITION(
        allocation_size % c_slot_size_bytes == 0,
        "Allocation size should be a multiple of slot size!");

    return allocation_size;
}

inline void base_memory_manager_t::validate_offset_alignment(address_offset_t memory_offset)
{
    ASSERT_PRECONDITION(
        memory_offset % c_slot_size_bytes == 0,
        "Misaligned memory offset!");
}

void base_memory_manager_t::validate_managed_memory_range() const
{
    std::stringstream message1;
    message1
        << "Total memory size (" << m_total_memory_size
        << ") is too large for start memory offset (" << m_start_memory_offset
        << ")!";
    ASSERT_PRECONDITION(
        m_start_memory_offset + m_total_memory_size > m_start_memory_offset,
        message1.str());

    auto base_memory_address_as_integer = reinterpret_cast<size_t>(m_base_memory_address);
    std::stringstream message2;
    message2
        << "Start memory offset (" << m_start_memory_offset
        << ") is too large for base memory address (" << base_memory_address_as_integer
        << ")!";
    ASSERT_PRECONDITION(
        base_memory_address_as_integer + m_start_memory_offset >= base_memory_address_as_integer,
        message2.str());

    size_t start_memory_address_as_integer = base_memory_address_as_integer + m_start_memory_offset;
    std::stringstream message3;
    message3
        << "Total memory size (" << m_total_memory_size
        << ") is too large for start memory address (" << start_memory_address_as_integer
        << ")!";
    ASSERT_PRECONDITION(
        start_memory_address_as_integer + m_total_memory_size > start_memory_address_as_integer,
        message3.str());
}

void base_memory_manager_t::validate_address(const uint8_t* const memory_address) const
{
    ASSERT_PRECONDITION(
        (reinterpret_cast<size_t>(memory_address)) % c_allocation_alignment == 0,
        "Memory address is misaligned!");

    ASSERT_PRECONDITION(
        memory_address >= m_base_memory_address + m_start_memory_offset
            && memory_address <= m_base_memory_address + m_start_memory_offset + m_total_memory_size,
        "Memory address is outside managed memory range!");
}

void base_memory_manager_t::validate_offset(address_offset_t memory_offset) const
{
    ASSERT_PRECONDITION(
        memory_offset != c_invalid_address_offset,
        "Memory offset is invalid!");

    validate_offset_alignment(memory_offset);

    ASSERT_PRECONDITION(
        memory_offset >= m_start_memory_offset
            && memory_offset <= m_start_memory_offset + m_total_memory_size,
        "Memory offset is outside managed memory range!");
}

void base_memory_manager_t::validate_slot_offset(slot_offset_t slot_offset) const
{
    ASSERT_PRECONDITION(
        slot_offset >= c_first_slot_offset && slot_offset <= c_last_slot_offset,
        "Slot offset is invalid!");
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

address_offset_t base_memory_manager_t::get_chunk_address_offset(address_offset_t memory_offset) const
{
    validate_offset_alignment(memory_offset);

    return memory_offset - (memory_offset % c_chunk_size_bytes);
}

chunk_offset_t base_memory_manager_t::get_chunk_offset(address_offset_t memory_offset) const
{
    validate_offset_alignment(memory_offset);

    return static_cast<chunk_offset_t>(memory_offset / c_chunk_size_bytes);
}

slot_offset_t base_memory_manager_t::get_slot_offset(address_offset_t memory_offset) const
{
    validate_offset_alignment(memory_offset);

    return static_cast<slot_offset_t>((memory_offset % c_chunk_size_bytes) / c_slot_size_bytes);
}
