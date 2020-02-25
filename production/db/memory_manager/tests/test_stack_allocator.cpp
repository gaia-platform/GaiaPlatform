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

void output_allocation_information(size_t size, ADDRESS_OFFSET offset)
{
    cout << endl << size << " bytes were allocated at offset " << offset << "." << endl;
}

void validate_allocation_record(
    stack_allocator* pStackAllocator,
    size_t allocationNumber,
    SLOT_ID expectedSlotId,
    ADDRESS_OFFSET expectedMemoryOffset,
    ADDRESS_OFFSET expectedOldMemoryOffset)
{
    stack_allocator_allocation* pStackAllocationRecord = pStackAllocator->get_allocation_record(allocationNumber);
    retail_assert(pStackAllocationRecord->slotId == expectedSlotId, "Allocation record has incorrect slot id!");
    retail_assert(pStackAllocationRecord->memory_offset == expectedMemoryOffset, "Allocation record has incorrect allocation offset!");
    retail_assert(pStackAllocationRecord->old_memory_offset == expectedOldMemoryOffset, "Allocation record has incorrect old allocation offset!");
}

void test_stack_allocator()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** StackAllocator tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    const size_t memorySize = 8000;
    const size_t minimumMainMemoryAvailableSize = 1000;
    uint8_t memory[memorySize];

    memory_manager memoryManager;

    gaia::db::memory_manager::error_code errorCode = not_set;

    execution_flags executionFlags;
    executionFlags.enable_extra_validations = true;
    executionFlags.enable_console_output = true;

    memoryManager.set_execution_flags(executionFlags);
    errorCode = memoryManager.manage(memory, memorySize, minimumMainMemoryAvailableSize, true);
    retail_assert(errorCode == success, "Manager initialization has failed!");
    cout << "PASSED: Manager initialization was successful!" << endl;

    size_t stackAllocatorMemorySize = 2000;

    stack_allocator* pStackAllocator = nullptr;
    errorCode = memoryManager.create_stack_allocator(stackAllocatorMemorySize, pStackAllocator);
    retail_assert(errorCode == success, "Stack allocator creation has failed!");

    size_t firstAllocationSize = 64;
    size_t secondAllocationSize = 256;
    size_t thirdAllocationSize = 128;
    size_t fourthAllocationSize = 24;
    size_t fifthAllocationSize = 72;

    SLOT_ID firstSlotId = 1;
    SLOT_ID secondSlotId = 2;
    SLOT_ID thirdSlotId = 3;
    SLOT_ID fourthSlotId = 4;
    SLOT_ID fifthSlotId = 5;

    ADDRESS_OFFSET firstOldOffset = 1024;
    ADDRESS_OFFSET secondOldOffset = 1032;
    ADDRESS_OFFSET thirdOldOffset = 1040;
    ADDRESS_OFFSET fourthOldOffset = 1048;
    ADDRESS_OFFSET fifthOldOffset = 1056;

    SLOT_ID deletedSlotId = 88;
    ADDRESS_OFFSET deletedOldOffset = 1080;

    ADDRESS_OFFSET firstAllocationOffset = 0;
    errorCode = pStackAllocator->allocate(firstSlotId, firstOldOffset, firstAllocationSize, firstAllocationOffset);
    retail_assert(errorCode == success, "First allocation has failed!");
    output_allocation_information(firstAllocationSize, firstAllocationOffset);
    validate_allocation_record(pStackAllocator, 1, firstSlotId, firstAllocationOffset, firstOldOffset);

    ADDRESS_OFFSET secondAllocationOffset = 0;
    errorCode = pStackAllocator->allocate(secondSlotId, secondOldOffset, secondAllocationSize, secondAllocationOffset);
    retail_assert(errorCode == success, "Second allocation has failed!");
    output_allocation_information(secondAllocationSize, secondAllocationOffset);
    validate_allocation_record(pStackAllocator, 2, secondSlotId, secondAllocationOffset, secondOldOffset);

    retail_assert(
        secondAllocationOffset == firstAllocationOffset + firstAllocationSize + sizeof(memory_allocation_metadata),
        "Second allocation offset does not have expected value.");

    ADDRESS_OFFSET thirdAllocationOffset = 0;
    errorCode = pStackAllocator->allocate(thirdSlotId, thirdOldOffset, thirdAllocationSize, thirdAllocationOffset);
    retail_assert(errorCode == success, "Third allocation has failed!");
    output_allocation_information(thirdAllocationSize, thirdAllocationOffset);
    validate_allocation_record(pStackAllocator, 3, thirdSlotId, thirdAllocationOffset, thirdOldOffset);

    retail_assert(
        thirdAllocationOffset == secondAllocationOffset + secondAllocationSize + sizeof(memory_allocation_metadata),
        "Third allocation offset does not have expected value.");

    retail_assert(pStackAllocator->get_allocation_count() == 3, "Allocation count is not the expected 3!");

    errorCode = pStackAllocator->deallocate(1);
    retail_assert(errorCode == success, "First deallocation has failed!");
    cout << endl << "Deallocate all but the first allocation." << endl;

    retail_assert(pStackAllocator->get_allocation_count() == 1, "Allocation count is not the expected 1!");

    ADDRESS_OFFSET fourthAllocationOffset = 0;
    errorCode = pStackAllocator->allocate(fourthSlotId, fourthOldOffset, fourthAllocationSize, fourthAllocationOffset);
    retail_assert(errorCode == success, "Fourth allocation has failed!");
    output_allocation_information(fourthAllocationSize, fourthAllocationOffset);
    validate_allocation_record(pStackAllocator, 2, fourthSlotId, fourthAllocationOffset, fourthOldOffset);

    retail_assert(
        fourthAllocationOffset == firstAllocationOffset + firstAllocationSize + sizeof(memory_allocation_metadata),
        "Fourth allocation offset does not have expected value.");

    retail_assert(pStackAllocator->get_allocation_count() == 2, "Allocation count is not the expected 2!");

    pStackAllocator->deallocate(deletedSlotId, deletedOldOffset);
    validate_allocation_record(pStackAllocator, 3, deletedSlotId, 0, deletedOldOffset);

    retail_assert(pStackAllocator->get_allocation_count() == 3, "Allocation count is not the expected 3!");

    pStackAllocator->deallocate(0);
    retail_assert(errorCode == success, "Second deallocation has failed!");
    cout << endl << "Deallocate all allocations." << endl;

    retail_assert(pStackAllocator->get_allocation_count() == 0, "Allocation count is not the expected 0!");

    ADDRESS_OFFSET fifthAllocationOffset = 0;
    pStackAllocator->allocate(fifthSlotId, fifthOldOffset, fifthAllocationSize, fifthAllocationOffset);
    retail_assert(errorCode == success, "Fifth allocation has failed!");
    output_allocation_information(fifthAllocationSize, fifthAllocationOffset);
    validate_allocation_record(pStackAllocator, 1, fifthSlotId, fifthAllocationOffset, fifthOldOffset);

    retail_assert(
        fifthAllocationOffset == firstAllocationOffset,
        "Fifth allocation offset does not have expected value.");

    retail_assert(pStackAllocator->get_allocation_count() == 1, "Allocation count is not the expected 1!");

    delete pStackAllocator;

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** StackAllocator tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
