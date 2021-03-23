/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "gaia_internal/common/retail_assert.hpp"

#include "chunk_manager.hpp"
#include "memory_manager.hpp"

using namespace std;

using namespace gaia::db::memory_manager;

TEST(memory_manager, basic_operation)
{
    // Allocate a bit more memory than necessary,
    // to allow bumping the starting pointer to the next aligned address.
    constexpr size_t c_memory_size = 16 * 1024 * 1024;
    std::vector<uint8_t> memory_vector(c_memory_size + c_allocation_alignment);
    uint8_t* memory = memory_vector.data();
    memory += c_allocation_alignment - (((size_t)memory) % c_allocation_alignment);

    memory_manager_t memory_manager;

    execution_flags_t execution_flags;
    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    memory_manager.manage(memory, c_memory_size);

    address_offset_t first_allocation_offset = memory_manager.allocate_chunk();
    ASSERT_NE(first_allocation_offset, c_invalid_address_offset);
    cout << "Chunk was allocated at offset " << first_allocation_offset << "." << endl;

    address_offset_t second_allocation_offset = memory_manager.allocate_chunk();
    ASSERT_NE(second_allocation_offset, c_invalid_address_offset);
    cout << "Chunk was allocated at offset " << second_allocation_offset << "." << endl;

    ASSERT_EQ(
        first_allocation_offset + c_chunk_size,
        second_allocation_offset);

    address_offset_t third_allocation_offset = memory_manager.allocate_chunk();
    ASSERT_NE(third_allocation_offset, c_invalid_address_offset);
    cout << "Chunk was allocated at offset " << third_allocation_offset << "." << endl;

    ASSERT_EQ(
        second_allocation_offset + c_chunk_size,
        third_allocation_offset);
}

void output_allocation_information(size_t requested_size, size_t allocated_size, address_offset_t offset)
{
    cout
        << endl
        << requested_size << " bytes were requested and "
        << allocated_size << " bytes were allocated at offset " << offset << "." << endl;
}

TEST(memory_manager, internal_allocate_operation)
{
    // Allocate a bit more memory than necessary,
    // to allow bumping the starting pointer to the next aligned address.
    constexpr size_t c_memory_size = 4 * 1024 * 1024;
    std::vector<uint8_t> memory_vector(c_memory_size + c_allocation_alignment);
    uint8_t* memory = memory_vector.data();
    memory += c_allocation_alignment - (((size_t)memory) % c_allocation_alignment);

    memory_manager_t memory_manager;

    execution_flags_t execution_flags;

    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    memory_manager.manage(memory, c_memory_size);

    constexpr size_t c_first_allocation_size = 64;
    constexpr size_t c_second_allocation_size = 250;
    constexpr size_t c_third_allocation_size = 128;

    size_t first_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_first_allocation_size);
    size_t second_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_second_allocation_size);
    size_t third_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_third_allocation_size);

    address_offset_t first_allocation_offset = memory_manager.allocate(c_first_allocation_size);
    ASSERT_NE(first_allocation_offset, c_invalid_address_offset);
    output_allocation_information(
        c_first_allocation_size, first_adjusted_allocation_size, first_allocation_offset);

    address_offset_t second_allocation_offset = memory_manager.allocate(c_second_allocation_size);
    ASSERT_NE(second_allocation_offset, c_invalid_address_offset);
    output_allocation_information(
        c_second_allocation_size, second_adjusted_allocation_size, second_allocation_offset);

    ASSERT_EQ(
        first_allocation_offset + first_adjusted_allocation_size,
        second_allocation_offset);

    address_offset_t third_allocation_offset = memory_manager.allocate(c_third_allocation_size);
    ASSERT_NE(third_allocation_offset, c_invalid_address_offset);
    output_allocation_information(
        c_third_allocation_size, third_adjusted_allocation_size, third_allocation_offset);

    ASSERT_EQ(
        second_allocation_offset + second_adjusted_allocation_size,
        third_allocation_offset);
}
