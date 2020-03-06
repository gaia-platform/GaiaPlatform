/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>

#include "gtest/gtest.h"

#include "stack_allocator.hpp"
#include "memory_manager.hpp"

using namespace std;

using namespace gaia::db::memory_manager;

void output_allocation_information(size_t size, address_offset_t offset)
{
    cout << endl << size << " bytes were allocated at offset " << offset << "." << endl;
}

TEST(memory_manager, basic_operation)
{
    const size_t memory_size = 8000;
    const size_t main_memory_system_reserved_size = 1000;
    uint8_t memory[memory_size];

    memory_manager_t memory_manager;

    execution_flags_t execution_flags;

    gaia::db::memory_manager::error_code_t error_code = error_code_t::not_set;

    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    error_code = memory_manager.manage(memory, memory_size, main_memory_system_reserved_size, true);
    ASSERT_EQ(error_code_t::success, error_code);

    size_t first_allocation_size = 64;
    size_t second_allocation_size = 256;
    size_t third_allocation_size = 128;

    address_offset_t first_allocation_offset = 0;
    error_code = memory_manager.allocate(first_allocation_size, first_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(first_allocation_size, first_allocation_offset);

    address_offset_t second_allocation_offset = 0;
    error_code = memory_manager.allocate(second_allocation_size, second_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(second_allocation_size, second_allocation_offset);

    ASSERT_EQ(
        first_allocation_offset + first_allocation_size + sizeof(memory_allocation_metadata_t),
        second_allocation_offset);

    address_offset_t third_allocation_offset = 0;
    error_code = memory_manager.allocate(third_allocation_size, third_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(third_allocation_size, third_allocation_offset);

    ASSERT_EQ(
        second_allocation_offset + second_allocation_size + sizeof(memory_allocation_metadata_t),
        third_allocation_offset);
}

TEST(memory_manager, advanced_operation)
{
    const size_t memory_size = 8000;
    const size_t main_memory_system_reserved_size = 1000;
    uint8_t memory[memory_size];

    memory_manager_t memory_manager;

    execution_flags_t execution_flags;

    gaia::db::memory_manager::error_code_t error_code = error_code_t::not_set;

    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    error_code = memory_manager.manage(memory, memory_size, main_memory_system_reserved_size, true);
    ASSERT_EQ(error_code_t::success, error_code);

    size_t stack_allocator_memory_size = 2000;

    // Make 3 allocations using a stack_allocator_t.
    stack_allocator_t* stack_allocator = nullptr;
    error_code = memory_manager.create_stack_allocator(stack_allocator_memory_size, stack_allocator);
    ASSERT_EQ(error_code_t::success, error_code);

    size_t first_allocation_size = 64;
    size_t second_allocation_size = 256;
    size_t third_allocation_size = 128;

    address_offset_t first_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, 0, first_allocation_size, first_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(first_allocation_size, first_allocation_offset);

    address_offset_t second_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, 0, second_allocation_size, second_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(second_allocation_size, second_allocation_offset);

    ASSERT_EQ(
        first_allocation_offset + first_allocation_size + sizeof(memory_allocation_metadata_t),
        second_allocation_offset);

    address_offset_t third_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, 0, third_allocation_size, third_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(third_allocation_size, third_allocation_offset);

    ASSERT_EQ(
        second_allocation_offset + second_allocation_size + sizeof(memory_allocation_metadata_t),
        third_allocation_offset);

    ASSERT_EQ(3, stack_allocator->get_allocation_count());

    // Commit stack allocator.
    size_t serialization_number = 331;
    cout << endl << "Commit first stack allocator with serialization number " << serialization_number << "..." << endl;
    error_code = memory_manager.commit_stack_allocator(stack_allocator, serialization_number);
    ASSERT_EQ(error_code_t::success, error_code);

    memory_list_node_t* first_read_of_list_head = nullptr;
    error_code = memory_manager.get_unserialized_allocations_list_head(first_read_of_list_head);
    ASSERT_EQ(error_code_t::success, error_code);
    ASSERT_NE(nullptr, first_read_of_list_head);

    // Make 2 more allocations using a new stack_allocator_t.
    // Both allocations will replace earlier allocations (4th replaces 2nd and 5th replaces 1st),
    // which will get garbage collected at commit time.
    error_code = memory_manager.create_stack_allocator(stack_allocator_memory_size, stack_allocator);
    ASSERT_EQ(error_code_t::success, error_code);

    size_t fourth_allocation_size = 256;
    size_t fifth_allocation_size = 64;

    address_offset_t fourth_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, second_allocation_offset, fourth_allocation_size, fourth_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(fourth_allocation_size, fourth_allocation_offset);

    address_offset_t fifth_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, first_allocation_offset, fifth_allocation_size, fifth_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(fifth_allocation_size, fifth_allocation_offset);

    ASSERT_EQ(
        fifth_allocation_offset,
        fourth_allocation_offset + fourth_allocation_size + sizeof(memory_allocation_metadata_t));

    // Commit stack allocator.
    serialization_number = 332;
    cout << endl << "Commit second stack allocator with serialization number " << serialization_number << "..." << endl;
    error_code = memory_manager.commit_stack_allocator(stack_allocator, serialization_number);
    ASSERT_EQ(error_code_t::success, error_code);

    memory_list_node_t* second_read_of_list_head = nullptr;
    error_code = memory_manager.get_unserialized_allocations_list_head(second_read_of_list_head);
    ASSERT_EQ(error_code_t::success, error_code);
    ASSERT_NE(nullptr, second_read_of_list_head);

    ASSERT_EQ(first_read_of_list_head->next, second_read_of_list_head->next);

    // Validate content of unserialized allocations list.
    ASSERT_NE(0, second_read_of_list_head->next);
    memory_record_t* first_unserialized_record = memory_manager.read_memory_record(second_read_of_list_head->next);
    ASSERT_NE(0, first_unserialized_record->next);
    address_offset_t second_unserialized_record_offset = first_unserialized_record->next;

    // Update unserialized allocations list.
    cout << endl << "Calling update_unserialized_allocations_list_head() with parameter: ";
    cout << second_unserialized_record_offset << "." << endl;
    error_code = memory_manager.update_unserialized_allocations_list_head(first_unserialized_record->next);
    ASSERT_EQ(error_code_t::success, error_code);

    memory_list_node_t* third_read_of_list_head = nullptr;
    error_code = memory_manager.get_unserialized_allocations_list_head(third_read_of_list_head);
    ASSERT_EQ(error_code_t::success, error_code);
    ASSERT_NE(nullptr, third_read_of_list_head);

    ASSERT_EQ(second_unserialized_record_offset, third_read_of_list_head->next);

    // Test allocating from freed memory.
    // First, we reclaim a full freed block.
    address_offset_t offset_first_free_allocation = 0;
    error_code = memory_manager.allocate(second_allocation_size, offset_first_free_allocation);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(second_allocation_size, offset_first_free_allocation);

    // Second, we reclaim a part of a freed block.
    address_offset_t offset_second_free_allocation = 0;
    error_code = memory_manager.allocate(third_allocation_size, offset_second_free_allocation);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(third_allocation_size, offset_second_free_allocation);
}
