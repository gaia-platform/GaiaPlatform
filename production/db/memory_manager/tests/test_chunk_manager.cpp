/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>

#include "gtest/gtest.h"

#include "chunk_manager.hpp"
#include "memory_manager.hpp"

using namespace std;

using namespace gaia::db::memory_manager;

void output_allocation_information(size_t requested_size, size_t allocated_size, address_offset_t offset)
{
    cout
        << endl
        << requested_size << " bytes were requested and "
        << allocated_size << " bytes were allocated at offset " << offset << "." << endl;
}

TEST(memory_manager, chunk_manager)
{
    // Allocate a bit more memory than necessary,
    // to allow bumping the starting pointer to the next aligned address.
    constexpr size_t c_memory_size = 4 * 1024 * 1024;
    std::vector<uint8_t> memory_vector(c_memory_size + c_allocation_alignment);
    uint8_t* memory = memory_vector.data();
    memory += c_allocation_alignment - (((size_t)memory) % c_allocation_alignment);

    address_offset_t memory_offset = c_invalid_address_offset;

    memory_manager_t memory_manager;

    execution_flags_t execution_flags;
    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    memory_manager.manage(memory, c_memory_size);

    unique_ptr<chunk_manager_t> chunk_manager = make_unique<chunk_manager_t>();
    chunk_manager->set_execution_flags(execution_flags);
    memory_offset = memory_manager.allocate_chunk();
    ASSERT_NE(memory_offset, c_invalid_address_offset);
    chunk_manager->initialize(memory, memory_offset);

    constexpr size_t c_first_allocation_size = 64;
    constexpr size_t c_second_allocation_size = 253;
    constexpr size_t c_third_allocation_size = 122;
    constexpr size_t c_fourth_allocation_size = 24;
    constexpr size_t c_fifth_allocation_size = 72;

    size_t first_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_first_allocation_size);
    size_t second_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_second_allocation_size);
    size_t third_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_third_allocation_size);
    size_t fourth_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_fourth_allocation_size);
    size_t fifth_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_fifth_allocation_size);

    address_offset_t first_allocation_offset = chunk_manager->allocate(c_first_allocation_size);
    ASSERT_NE(first_allocation_offset, c_invalid_address_offset);
    output_allocation_information(
        c_first_allocation_size, first_adjusted_allocation_size, first_allocation_offset);

    address_offset_t second_allocation_offset = chunk_manager->allocate(c_second_allocation_size);
    ASSERT_NE(second_allocation_offset, c_invalid_address_offset);
    output_allocation_information(
        c_second_allocation_size, second_adjusted_allocation_size, second_allocation_offset);

    ASSERT_EQ(
        first_allocation_offset + first_adjusted_allocation_size,
        second_allocation_offset);

    address_offset_t third_allocation_offset = chunk_manager->allocate(c_third_allocation_size);
    ASSERT_NE(third_allocation_offset, c_invalid_address_offset);
    output_allocation_information(
        c_third_allocation_size, third_adjusted_allocation_size, third_allocation_offset);

    ASSERT_EQ(
        second_allocation_offset + second_adjusted_allocation_size,
        third_allocation_offset);

    cout
        << endl
        << "Rollback allocations." << endl;
    chunk_manager->rollback();

    address_offset_t fourth_allocation_offset = chunk_manager->allocate(c_fourth_allocation_size);
    ASSERT_NE(fourth_allocation_offset, c_invalid_address_offset);
    output_allocation_information(
        c_fourth_allocation_size, fourth_adjusted_allocation_size, fourth_allocation_offset);

    ASSERT_EQ(
        first_allocation_offset,
        fourth_allocation_offset);

    cout
        << endl
        << "Commit allocations." << endl;
    chunk_manager->commit();

    // Create a new chunk manager that reads the allocations made using the original manager.
    unique_ptr<chunk_manager_t> new_chunk_manager = make_unique<chunk_manager_t>();
    new_chunk_manager->set_execution_flags(execution_flags);
    new_chunk_manager->load(memory, memory_offset);

    address_offset_t fifth_allocation_offset = new_chunk_manager->allocate(c_fifth_allocation_size);
    ASSERT_NE(fifth_allocation_offset, c_invalid_address_offset);
    output_allocation_information(
        c_fifth_allocation_size, fifth_adjusted_allocation_size, fifth_allocation_offset);

    ASSERT_EQ(
        fourth_allocation_offset + fourth_adjusted_allocation_size,
        fifth_allocation_offset);
}
