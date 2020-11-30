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

void validate_allocation_record(
    unique_ptr<stack_allocator_t>& stack_allocator,
    size_t allocation_number,
    slot_id_t expected_slot_id,
    address_offset_t expected_memory_offset,
    address_offset_t expected_old_memory_offset)
{
    stack_allocator_allocation_t* stack_allocation_record = stack_allocator->get_allocation_record(allocation_number);
    ASSERT_EQ(expected_slot_id, stack_allocation_record->slot_id);
    ASSERT_EQ(expected_memory_offset, stack_allocation_record->memory_offset);
    ASSERT_EQ(expected_old_memory_offset, stack_allocation_record->old_memory_offset);
}

TEST(memory_manager, stack_allocator)
{
    constexpr size_t c_memory_size = 8000;
    uint8_t memory[c_memory_size];
    address_offset_t memory_offset = c_invalid_offset;

    memory_manager_t memory_manager;

    gaia::db::memory_manager::error_code_t error_code = error_code_t::not_set;

    execution_flags_t execution_flags;
    execution_flags.enable_console_output = true;

    memory_manager.set_execution_flags(execution_flags);
    error_code = memory_manager.manage(memory, c_memory_size);
    ASSERT_EQ(error_code_t::success, error_code);

    constexpr size_t c_stack_allocator_memory_size = 2000;

    unique_ptr<stack_allocator_t> stack_allocator = make_unique<stack_allocator_t>();
    stack_allocator->set_execution_flags(execution_flags);
    error_code = memory_manager.allocate_raw(c_stack_allocator_memory_size, memory_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    error_code = stack_allocator->initialize(memory, memory_offset, c_stack_allocator_memory_size);
    ASSERT_EQ(error_code_t::success, error_code);

    constexpr size_t c_first_allocation_size = 64;
    constexpr size_t c_second_allocation_size = 253;
    constexpr size_t c_third_allocation_size = 122;
    constexpr size_t c_fourth_allocation_size = 24;
    constexpr size_t c_fifth_allocation_size = 72;

    constexpr slot_id_t c_first_slot_id = 1;
    constexpr slot_id_t c_second_slot_id = 2;
    constexpr slot_id_t c_third_slot_id = 3;
    constexpr slot_id_t c_fourth_slot_id = 4;
    constexpr slot_id_t c_fifth_slot_id = 5;

    constexpr address_offset_t c_first_old_offset = 1024;
    constexpr address_offset_t c_second_old_offset = 1032;
    constexpr address_offset_t c_third_old_offset = 1040;
    constexpr address_offset_t c_fourth_old_offset = 1048;
    constexpr address_offset_t c_fifth_old_offset = 1056;

    constexpr slot_id_t c_deleted_slot_id = 88;
    constexpr address_offset_t c_deleted_old_offset = 1080;

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

    address_offset_t first_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(
        c_first_slot_id, c_first_old_offset, c_first_allocation_size, first_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_first_allocation_size, first_adjusted_allocation_size, first_allocation_offset);
    validate_allocation_record(stack_allocator, 1, c_first_slot_id, first_allocation_offset, c_first_old_offset);

    address_offset_t second_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(
        c_second_slot_id, c_second_old_offset, c_second_allocation_size, second_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_second_allocation_size, second_adjusted_allocation_size, second_allocation_offset);
    validate_allocation_record(stack_allocator, 2, c_second_slot_id, second_allocation_offset, c_second_old_offset);

    ASSERT_EQ(
        first_allocation_offset + first_adjusted_allocation_size + sizeof(memory_allocation_metadata_t),
        second_allocation_offset);

    address_offset_t third_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(
        c_third_slot_id, c_third_old_offset, c_third_allocation_size, third_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_third_allocation_size, third_adjusted_allocation_size, third_allocation_offset);
    validate_allocation_record(stack_allocator, 3, c_third_slot_id, third_allocation_offset, c_third_old_offset);

    ASSERT_EQ(
        second_allocation_offset + second_adjusted_allocation_size + sizeof(memory_allocation_metadata_t),
        third_allocation_offset);

    ASSERT_EQ(3, stack_allocator->get_allocation_count());

    cout << endl
         << "Deallocate all but the first allocation." << endl;
    error_code = stack_allocator->deallocate(1);
    ASSERT_EQ(error_code_t::success, error_code);

    ASSERT_EQ(1, stack_allocator->get_allocation_count());

    address_offset_t fourth_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(
        c_fourth_slot_id, c_fourth_old_offset, c_fourth_allocation_size, fourth_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_fourth_allocation_size, fourth_adjusted_allocation_size, fourth_allocation_offset);
    validate_allocation_record(stack_allocator, 2, c_fourth_slot_id, fourth_allocation_offset, c_fourth_old_offset);

    ASSERT_EQ(
        first_allocation_offset + first_adjusted_allocation_size + sizeof(memory_allocation_metadata_t),
        fourth_allocation_offset);

    ASSERT_EQ(2, stack_allocator->get_allocation_count());

    stack_allocator->deallocate(c_deleted_slot_id, c_deleted_old_offset);
    validate_allocation_record(stack_allocator, 3, c_deleted_slot_id, c_invalid_offset, c_deleted_old_offset);

    ASSERT_EQ(3, stack_allocator->get_allocation_count());

    cout << endl
         << "Deallocate all allocations." << endl;
    stack_allocator->deallocate(0);
    ASSERT_EQ(error_code_t::success, error_code);

    ASSERT_EQ(0, stack_allocator->get_allocation_count());

    address_offset_t fifth_allocation_offset = c_invalid_offset;
    error_code = stack_allocator->allocate(
        c_fifth_slot_id, c_fifth_old_offset, c_fifth_allocation_size, fifth_allocation_offset);
    ASSERT_EQ(error_code_t::success, error_code);
    output_allocation_information(
        c_fifth_allocation_size, fifth_adjusted_allocation_size, fifth_allocation_offset);
    validate_allocation_record(stack_allocator, 1, c_fifth_slot_id, fifth_allocation_offset, c_fifth_old_offset);

    ASSERT_EQ(first_allocation_offset, fifth_allocation_offset);

    ASSERT_EQ(1, stack_allocator->get_allocation_count());

    // Create a stack allocator that reads the allocations made by the original allocator.
    unique_ptr<stack_allocator_t> read_stack_allocator = make_unique<stack_allocator_t>();
    read_stack_allocator->set_execution_flags(execution_flags);
    error_code = read_stack_allocator->load(memory, memory_offset, c_stack_allocator_memory_size);
    ASSERT_EQ(error_code_t::success, error_code);

    // Verify that the allocator is seeing our 2 earlier allocations.
    ASSERT_EQ(1, read_stack_allocator->get_allocation_count());
}
