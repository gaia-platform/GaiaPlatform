/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "gaia_internal/common/assert.hpp"

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
    memory_manager.initialize(memory, c_memory_size);

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

    memory_manager.deallocate_chunk(second_allocation_offset);
    cout << "Chunk was deallocated at offset " << second_allocation_offset << "." << endl;

    address_offset_t fourth_allocation_offset = memory_manager.allocate_chunk();
    ASSERT_NE(fourth_allocation_offset, c_invalid_address_offset);
    cout << "Chunk was allocated at offset " << fourth_allocation_offset << "." << endl;

    ASSERT_EQ(
        second_allocation_offset,
        fourth_allocation_offset);
}
