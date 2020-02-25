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

void test_stack_allocator();

int main()
{
    test_stack_allocator();

    cout << endl << c_all_tests_passed << endl;
}

void output_allocation_information(size_t size, address_offset_t offset)
{
    cout << endl << size << " bytes were allocated at offset " << offset << "." << endl;
}

void validate_allocation_record(
    stack_allocator_t* stack_allocator,
    size_t allocation_number,
    slot_id_t expected_slot_id,
    address_offset_t expected_memory_offset,
    address_offset_t expected_old_memory_offset)
{
    stack_allocator_allocation_t* pStackAllocationRecord = stack_allocator->get_allocation_record(allocation_number);
    retail_assert(pStackAllocationRecord->slot_id == expected_slot_id, "Allocation record has incorrect slot id!");
    retail_assert(pStackAllocationRecord->memory_offset == expected_memory_offset, "Allocation record has incorrect allocation offset!");
    retail_assert(pStackAllocationRecord->old_memory_offset == expected_old_memory_offset, "Allocation record has incorrect old allocation offset!");
}

void test_stack_allocator()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** stack_allocator_t tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    const size_t memory_size = 8000;
    const size_t main_memory_system_reserved_size = 1000;
    uint8_t memory[memory_size];

    memory_manager_t memory_manager;

    gaia::db::memory_manager::error_code_t error_code = not_set;

    execution_flags_t execution_flags;
    execution_flags.enable_extra_validations = true;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    error_code = memory_manager.manage(memory, memory_size, main_memory_system_reserved_size, true);
    retail_assert(error_code == success, "Manager initialization has failed!");
    cout << "PASSED: Manager initialization was successful!" << endl;

    size_t stack_allocator_memory_size = 2000;

    stack_allocator_t* stack_allocator = nullptr;
    error_code = memory_manager.create_stack_allocator(stack_allocator_memory_size, stack_allocator);
    retail_assert(error_code == success, "Stack allocator creation has failed!");

    size_t first_allocation_size = 64;
    size_t second_allocation_size = 256;
    size_t third_allocation_size = 128;
    size_t fourth_allocation_size = 24;
    size_t fifth_allocation_size = 72;

    slot_id_t first_slot_id = 1;
    slot_id_t second_slot_id = 2;
    slot_id_t third_slot_id = 3;
    slot_id_t fourth_slot_id = 4;
    slot_id_t fifth_slot_id = 5;

    address_offset_t first_old_offset = 1024;
    address_offset_t second_old_offset = 1032;
    address_offset_t third_old_offset = 1040;
    address_offset_t fourth_old_offset = 1048;
    address_offset_t fifth_old_offset = 1056;

    slot_id_t deleted_slot_id = 88;
    address_offset_t deleted_old_offset = 1080;

    address_offset_t first_allocation_offset = 0;
    error_code = stack_allocator->allocate(first_slot_id, first_old_offset, first_allocation_size, first_allocation_offset);
    retail_assert(error_code == success, "First allocation has failed!");
    output_allocation_information(first_allocation_size, first_allocation_offset);
    validate_allocation_record(stack_allocator, 1, first_slot_id, first_allocation_offset, first_old_offset);

    address_offset_t second_allocation_offset = 0;
    error_code = stack_allocator->allocate(second_slot_id, second_old_offset, second_allocation_size, second_allocation_offset);
    retail_assert(error_code == success, "Second allocation has failed!");
    output_allocation_information(second_allocation_size, second_allocation_offset);
    validate_allocation_record(stack_allocator, 2, second_slot_id, second_allocation_offset, second_old_offset);

    retail_assert(
        second_allocation_offset == first_allocation_offset + first_allocation_size + sizeof(memory_allocation_metadata_t),
        "Second allocation offset does not have expected value.");

    address_offset_t third_allocation_offset = 0;
    error_code = stack_allocator->allocate(third_slot_id, third_old_offset, third_allocation_size, third_allocation_offset);
    retail_assert(error_code == success, "Third allocation has failed!");
    output_allocation_information(third_allocation_size, third_allocation_offset);
    validate_allocation_record(stack_allocator, 3, third_slot_id, third_allocation_offset, third_old_offset);

    retail_assert(
        third_allocation_offset == second_allocation_offset + second_allocation_size + sizeof(memory_allocation_metadata_t),
        "Third allocation offset does not have expected value.");

    retail_assert(stack_allocator->get_allocation_count() == 3, "Allocation count is not the expected 3!");

    error_code = stack_allocator->deallocate(1);
    retail_assert(error_code == success, "First deallocation has failed!");
    cout << endl << "Deallocate all but the first allocation." << endl;

    retail_assert(stack_allocator->get_allocation_count() == 1, "Allocation count is not the expected 1!");

    address_offset_t fourth_allocation_offset = 0;
    error_code = stack_allocator->allocate(fourth_slot_id, fourth_old_offset, fourth_allocation_size, fourth_allocation_offset);
    retail_assert(error_code == success, "Fourth allocation has failed!");
    output_allocation_information(fourth_allocation_size, fourth_allocation_offset);
    validate_allocation_record(stack_allocator, 2, fourth_slot_id, fourth_allocation_offset, fourth_old_offset);

    retail_assert(
        fourth_allocation_offset == first_allocation_offset + first_allocation_size + sizeof(memory_allocation_metadata_t),
        "Fourth allocation offset does not have expected value.");

    retail_assert(stack_allocator->get_allocation_count() == 2, "Allocation count is not the expected 2!");

    stack_allocator->deallocate(deleted_slot_id, deleted_old_offset);
    validate_allocation_record(stack_allocator, 3, deleted_slot_id, 0, deleted_old_offset);

    retail_assert(stack_allocator->get_allocation_count() == 3, "Allocation count is not the expected 3!");

    stack_allocator->deallocate(0);
    retail_assert(error_code == success, "Second deallocation has failed!");
    cout << endl << "Deallocate all allocations." << endl;

    retail_assert(stack_allocator->get_allocation_count() == 0, "Allocation count is not the expected 0!");

    address_offset_t fifth_allocation_offset = 0;
    stack_allocator->allocate(fifth_slot_id, fifth_old_offset, fifth_allocation_size, fifth_allocation_offset);
    retail_assert(error_code == success, "Fifth allocation has failed!");
    output_allocation_information(fifth_allocation_size, fifth_allocation_offset);
    validate_allocation_record(stack_allocator, 1, fifth_slot_id, fifth_allocation_offset, fifth_old_offset);

    retail_assert(
        fifth_allocation_offset == first_allocation_offset,
        "Fifth allocation offset does not have expected value.");

    retail_assert(stack_allocator->get_allocation_count() == 1, "Allocation count is not the expected 1!");

    delete stack_allocator;

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** stack_allocator_t tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
