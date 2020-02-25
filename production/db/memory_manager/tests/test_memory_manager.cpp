/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>

#include "constants.hpp"
#include "retail_assert.hpp"

#include "stack_allocator.hpp"
#include "memory_manager.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db::memory_manager;

void test_memory_manager_basic_operation();
void test_memory_manager_advanced_operation();

int main()
{
    test_memory_manager_basic_operation();

    test_memory_manager_advanced_operation();

    cout << endl << c_all_tests_passed << endl;
}

void output_allocation_information(size_t size, address_offset_t offset)
{
    cout << endl << size << " bytes were allocated at offset " << offset << "." << endl;
}

void test_memory_manager_basic_operation()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** memory_manager_t basic operation tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    const size_t memory_size = 8000;
    const size_t main_memory_system_reserved_size = 1000;
    uint8_t memory[memory_size];

    memory_manager_t memory_manager;

    execution_flags_t execution_flags;

    gaia::db::memory_manager::error_code_t error_code = not_set;

    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    error_code = memory_manager.manage(memory, memory_size, main_memory_system_reserved_size, true);
    retail_assert(error_code == success, "Manager initialization has failed!");
    cout << "PASSED: Manager initialization was successful!" << endl;

    size_t first_allocation_size = 64;
    size_t second_allocation_size = 256;
    size_t third_allocation_size = 128;

    address_offset_t first_allocation_offset = 0;
    error_code = memory_manager.allocate(first_allocation_size, first_allocation_offset);
    retail_assert(error_code == success, "First allocation has failed!");
    output_allocation_information(first_allocation_size, first_allocation_offset);

    address_offset_t second_allocation_offset = 0;
    error_code = memory_manager.allocate(second_allocation_size, second_allocation_offset);
    retail_assert(error_code == success, "Second allocation has failed!");
    output_allocation_information(second_allocation_size, second_allocation_offset);

    retail_assert(
        second_allocation_offset == first_allocation_offset + first_allocation_size + sizeof(memory_allocation_metadata_t), 
        "Second allocation offset is not the expected value.");

    address_offset_t third_allocation_offset = 0;
    error_code = memory_manager.allocate(third_allocation_size, third_allocation_offset);
    retail_assert(error_code == success, "Third allocation has failed!");
    output_allocation_information(third_allocation_size, third_allocation_offset);

    retail_assert(
        third_allocation_offset == second_allocation_offset + second_allocation_size + sizeof(memory_allocation_metadata_t), 
        "Third allocation offset is not the expected value.");

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** memory_manager_t basic operation tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}

void test_memory_manager_advanced_operation()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** memory_manager_t advanced operation tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    const size_t memory_size = 8000;
    const size_t main_memory_system_reserved_size = 1000;
    uint8_t memory[memory_size];

    memory_manager_t memory_manager;

    execution_flags_t execution_flags;

    gaia::db::memory_manager::error_code_t error_code = not_set;

    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    error_code = memory_manager.manage(memory, memory_size, main_memory_system_reserved_size, true);
    retail_assert(error_code == success, "Manager initialization has failed!");
    cout << "PASSED: Manager initialization was successful!" << endl;

    size_t stack_allocator_memory_size = 2000;

    // Make 3 allocations using a stack_allocator_t.
    stack_allocator_t* stack_allocator = nullptr;
    error_code = memory_manager.create_stack_allocator(stack_allocator_memory_size, stack_allocator);
    retail_assert(error_code == success, "First stack allocator creation has failed!");

    size_t first_allocation_size = 64;
    size_t second_allocation_size = 256;
    size_t third_allocation_size = 128;

    address_offset_t first_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, 0, first_allocation_size, first_allocation_offset);
    retail_assert(error_code == success, "First allocation has failed!");
    output_allocation_information(first_allocation_size, first_allocation_offset);

    address_offset_t second_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, 0, second_allocation_size, second_allocation_offset);
    retail_assert(error_code == success, "Second allocation has failed!");
    output_allocation_information(second_allocation_size, second_allocation_offset);

    retail_assert(
        second_allocation_offset == first_allocation_offset + first_allocation_size + sizeof(memory_allocation_metadata_t),
        "Second allocation offset does not have expected value.");

    address_offset_t third_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, 0, third_allocation_size, third_allocation_offset);
    retail_assert(error_code == success, "Third allocation has failed!");
    output_allocation_information(third_allocation_size, third_allocation_offset);

    retail_assert(
        third_allocation_offset == second_allocation_offset + second_allocation_size + sizeof(memory_allocation_metadata_t),
        "Third allocation offset does not have expected value.");

    retail_assert(stack_allocator->get_allocation_count() == 3, "Allocation count is not the expected 3!");

    // Commit stack allocator.
    size_t serialization_number = 331;
    cout << endl << "Commit first stack allocator with serialization number " << serialization_number << "..." << endl;
    error_code = memory_manager.commit_stack_allocator(stack_allocator, serialization_number);
    retail_assert(error_code == success, "First stack allocator commit has failed!");

    memory_list_node_t* first_read_of_list_head = nullptr;
    error_code = memory_manager.get_unserialized_allocations_list_head(first_read_of_list_head);
    retail_assert(error_code == success, "Failed first call to get unserialized allocations list head!");
    retail_assert(first_read_of_list_head != nullptr, "Failed the first attempt to retrieve a valid unserialized allocations list head.");

    // Make 2 more allocations using a new stack_allocator_t.
    // Both allocations will replace earlier allocations (4th replaces 2nd and 5th replaces 1st),
    // which will get garbage collected at commit time.
    error_code = memory_manager.create_stack_allocator(stack_allocator_memory_size, stack_allocator);
    retail_assert(error_code == success, "Second stack allocator creation has failed!");

    size_t fourth_allocation_size = 256;
    size_t fifth_allocation_size = 64;

    address_offset_t fourth_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, second_allocation_offset, fourth_allocation_size, fourth_allocation_offset);
    retail_assert(error_code == success, "Fourth allocation has failed!");
    output_allocation_information(fourth_allocation_size, fourth_allocation_offset);

    address_offset_t fifth_allocation_offset = 0;
    error_code = stack_allocator->allocate(0, first_allocation_offset, fifth_allocation_size, fifth_allocation_offset);
    retail_assert(error_code == success, "Fifth allocation has failed!");
    output_allocation_information(fifth_allocation_size, fifth_allocation_offset);

    retail_assert(
        fourth_allocation_offset + fourth_allocation_size + sizeof(memory_allocation_metadata_t) == fifth_allocation_offset,
        "Second allocation offset does not have expected value.");

    // Commit stack allocator.
    serialization_number = 332;
    cout << endl << "Commit second stack allocator with serialization number " << serialization_number << "..." << endl;
    error_code = memory_manager.commit_stack_allocator(stack_allocator, serialization_number);
    retail_assert(error_code == success, "Second stack allocator commit has failed!");

    memory_list_node_t* second_read_of_list_head = nullptr;
    error_code = memory_manager.get_unserialized_allocations_list_head(second_read_of_list_head);
    retail_assert(error_code == success, "Failed second call to get unserialized allocations list head!");
    retail_assert(second_read_of_list_head != nullptr, "Failed the second attempt to retrieve a valid unserialized allocations list head.");

    retail_assert(
        second_read_of_list_head->next == first_read_of_list_head->next,
        "First node in unserialized allocations list should not have changed!");

    // Validate content of unserialized allocations list.
    retail_assert(
        second_read_of_list_head->next != 0,
        "The unserialized allocations list should contain a first entry!");
    memory_record_t* first_unserialized_record = memory_manager.read_memory_record(second_read_of_list_head->next);
    retail_assert(
        first_unserialized_record->next != 0,
        "The unserialized allocations list should contain a second entry!");
    address_offset_t second_unserialized_record_offset = first_unserialized_record->next;

    // Update unserialized allocations list.
    cout << endl << "Calling update_unserialized_allocations_list_head() with parameter: ";
    cout << second_unserialized_record_offset << "." << endl;
    error_code = memory_manager.update_unserialized_allocations_list_head(first_unserialized_record->next);
    retail_assert(error_code == success, "Failed first call to update_unserialized_allocations_list_head()!");

    memory_list_node_t* third_read_of_list_head = nullptr;
    error_code = memory_manager.get_unserialized_allocations_list_head(third_read_of_list_head);
    retail_assert(error_code == success, "Failed third call to get unserialized allocations list head!");
    retail_assert(third_read_of_list_head != nullptr, "Failed the third attempt to retrieve a valid unserialized allocations list head.");

    retail_assert(
        third_read_of_list_head->next == second_unserialized_record_offset,
        "The value of the first node of the unserialized allocations list is not the expected one!");

    // Test allocating from freed memory.
    // First, we reclaim a full freed block.
    address_offset_t offset_first_free_allocation = 0;
    error_code = memory_manager.allocate(second_allocation_size, offset_first_free_allocation);
    retail_assert(error_code == success, "First free allocation has failed!");
    output_allocation_information(second_allocation_size, offset_first_free_allocation);

    // Second, we reclaim a part of a freed block.
    address_offset_t offset_second_free_allocation = 0;
    error_code = memory_manager.allocate(third_allocation_size, offset_second_free_allocation);
    retail_assert(error_code == success, "Second free allocation has failed!");
    output_allocation_information(third_allocation_size, offset_second_free_allocation);

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** memory_manager_t advanced operation tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
