/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>

#include "gtest/gtest.h"

#include "memory_manager.hpp"
#include "stack_allocator.hpp"

using namespace std;

using namespace gaia::db::memory_manager;

void output_allocation_information(size_t requested_size, size_t allocated_size, address_offset_t offset)
{
    cout << endl
         << requested_size << " bytes were requested and "
         << allocated_size << " bytes were allocated at offset " << offset << "." << endl;
}

TEST(memory_manager, basic_operation)
{
    constexpr size_t c_memory_size = 8000;
    uint8_t memory[c_memory_size];

    memory_manager_t memory_manager;

    execution_flags_t execution_flags;

    gaia::db::memory_manager::error_code_t error_code = error_code_t::not_set;

    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    error_code = memory_manager.manage(memory, c_memory_size);
    ASSERT_EQ(error_code_t::success, error_code);

    constexpr size_t c_first_allocation_size = 64;
    constexpr size_t c_second_allocation_size = 250;
    constexpr size_t c_third_allocation_size = 128;

    size_t first_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_first_allocation_size);
    size_t second_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_second_allocation_size);
    size_t third_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_third_allocation_size);

    address_offset_t first_allocation_offset = c_invalid_offset;
    error_code = memory_manager.allocate(c_first_allocation_size, first_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_first_allocation_size, first_adjusted_allocation_size, first_allocation_offset);

    address_offset_t second_allocation_offset = c_invalid_offset;
    error_code = memory_manager.allocate(c_second_allocation_size, second_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_second_allocation_size, second_adjusted_allocation_size, second_allocation_offset);

    ASSERT_EQ(
        first_allocation_offset + first_adjusted_allocation_size + sizeof(memory_allocation_metadata_t),
        second_allocation_offset);

    address_offset_t third_allocation_offset = c_invalid_offset;
    error_code = memory_manager.allocate(c_third_allocation_size, third_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_third_allocation_size, third_adjusted_allocation_size, third_allocation_offset);

    ASSERT_EQ(
        second_allocation_offset + second_adjusted_allocation_size + sizeof(memory_allocation_metadata_t),
        third_allocation_offset);
}

TEST(memory_manager, advanced_operation)
{
    constexpr size_t c_memory_size = 8000;
    uint8_t memory[c_memory_size];
    address_offset_t memory_offset = c_invalid_offset;

    memory_manager_t memory_manager;

    execution_flags_t execution_flags;

    gaia::db::memory_manager::error_code_t error_code = error_code_t::not_set;

    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    error_code = memory_manager.manage(memory, c_memory_size);
    ASSERT_EQ(error_code_t::success, error_code);

    constexpr size_t c_stack_allocator_memory_size = 2000;

    // Make 3 allocations using a stack_allocator_t.
    unique_ptr<stack_allocator_t> stack_allocator = make_unique<stack_allocator_t>();
    stack_allocator->set_execution_flags(execution_flags);
    error_code = memory_manager.allocate_raw(c_stack_allocator_memory_size, memory_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    error_code = stack_allocator->initialize(memory, memory_offset, c_stack_allocator_memory_size);
    ASSERT_EQ(error_code_t::success, error_code);

    constexpr size_t c_first_allocation_size = 64;
    constexpr size_t c_second_allocation_size = 250;
    constexpr size_t c_third_allocation_size = 128;

    size_t first_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_first_allocation_size);
    size_t second_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_second_allocation_size);
    size_t third_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_third_allocation_size);

    address_offset_t first_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(0, c_invalid_offset, c_first_allocation_size, first_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_first_allocation_size, first_adjusted_allocation_size, first_allocation_offset);

    address_offset_t second_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(0, c_invalid_offset, c_second_allocation_size, second_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_second_allocation_size, second_adjusted_allocation_size, second_allocation_offset);

    ASSERT_EQ(
        first_allocation_offset + first_adjusted_allocation_size + sizeof(memory_allocation_metadata_t),
        second_allocation_offset);

    address_offset_t third_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(0, c_invalid_offset, c_third_allocation_size, third_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_third_allocation_size, third_adjusted_allocation_size, third_allocation_offset);

    ASSERT_EQ(
        second_allocation_offset + second_adjusted_allocation_size + sizeof(memory_allocation_metadata_t),
        third_allocation_offset);

    ASSERT_EQ(3, stack_allocator->get_allocation_count());

    // Free stack allocator.
    cout << endl
         << "Free first stack allocator..." << endl;
    error_code = memory_manager.free_stack_allocator(stack_allocator);
    ASSERT_EQ(error_code_t::success, error_code);

    // Make 2 more allocations using a new stack_allocator_t.
    // Both allocations will replace earlier allocations (4th replaces 2nd and 5th replaces 1st),
    // which will get garbage collected at commit time.
    stack_allocator = make_unique<stack_allocator_t>();
    stack_allocator->set_execution_flags(execution_flags);
    error_code = memory_manager.allocate_raw(c_stack_allocator_memory_size, memory_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    error_code = stack_allocator->initialize(memory, memory_offset, c_stack_allocator_memory_size);
    ASSERT_EQ(error_code_t::success, error_code);

    constexpr size_t c_fourth_allocation_size = 256;
    constexpr size_t c_fifth_allocation_size = 66;

    size_t fourth_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_fourth_allocation_size);
    size_t fifth_adjusted_allocation_size
        = base_memory_manager_t::calculate_allocation_size(c_fifth_allocation_size);

    address_offset_t fourth_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(0, second_allocation_offset, c_fourth_allocation_size, fourth_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_fourth_allocation_size, fourth_adjusted_allocation_size, fourth_allocation_offset);

    address_offset_t fifth_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(0, first_allocation_offset, c_fifth_allocation_size, fifth_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_fifth_allocation_size, fifth_adjusted_allocation_size, fifth_allocation_offset);

    ASSERT_EQ(
        fourth_allocation_offset + fourth_adjusted_allocation_size + sizeof(memory_allocation_metadata_t),
        fifth_allocation_offset);

    // Free stack allocator.
    cout << endl
         << "Free second stack allocator..." << endl;
    error_code = memory_manager.free_stack_allocator(stack_allocator);
    ASSERT_EQ(error_code_t::success, error_code);

    // Verify that double freeing fails.
    error_code = memory_manager.free_stack_allocator(stack_allocator);
    ASSERT_EQ(error_code_t::invalid_argument_value, error_code);

    // Test allocating from freed memory.
    // First, we reclaim a full freed block.
    address_offset_t offset_first_free_allocation = c_invalid_offset;
    error_code = memory_manager.allocate(c_second_allocation_size, offset_first_free_allocation);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_second_allocation_size, second_adjusted_allocation_size, offset_first_free_allocation);

    // Second, we reclaim a part of a freed block.
    address_offset_t offset_second_free_allocation = c_invalid_offset;
    error_code = memory_manager.allocate(c_third_allocation_size, offset_second_free_allocation);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_third_allocation_size, third_adjusted_allocation_size, offset_second_free_allocation);
}
