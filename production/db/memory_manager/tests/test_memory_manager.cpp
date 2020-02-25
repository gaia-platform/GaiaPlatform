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

void output_allocation_information(size_t size, ADDRESS_OFFSET offset)
{
    cout << endl << size << " bytes were allocated at offset " << offset << "." << endl;
}

void test_memory_manager_basic_operation()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** MemoryManager basic operation tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    const size_t memorySize = 8000;
    const size_t mainMemorySystemReservedSize = 1000;
    uint8_t memory[memorySize];

    memory_manager memoryManager;

    execution_flags executionFlags;

    gaia::db::memory_manager::error_code errorCode = not_set;

    executionFlags.enable_extra_validations = true;
    executionFlags.enable_console_output = true;

    memoryManager.set_execution_flags(executionFlags);
    errorCode = memoryManager.manage(memory, memorySize, mainMemorySystemReservedSize, true);
    retail_assert(errorCode == success, "Manager initialization has failed!");
    cout << "PASSED: Manager initialization was successful!" << endl;

    size_t firstAllocationSize = 64;
    size_t secondAllocationSize = 256;
    size_t thirdAllocationSize = 128;

    ADDRESS_OFFSET firstAllocationOffset = 0;
    errorCode = memoryManager.allocate(firstAllocationSize, firstAllocationOffset);
    retail_assert(errorCode == success, "First allocation has failed!");
    output_allocation_information(firstAllocationSize, firstAllocationOffset);

    ADDRESS_OFFSET secondAllocationOffset = 0;
    errorCode = memoryManager.allocate(secondAllocationSize, secondAllocationOffset);
    retail_assert(errorCode == success, "Second allocation has failed!");
    output_allocation_information(secondAllocationSize, secondAllocationOffset);

    retail_assert(
        secondAllocationOffset == firstAllocationOffset + firstAllocationSize + sizeof(memory_allocation_metadata), 
        "Second allocation offset is not the expected value.");

    ADDRESS_OFFSET thirdAllocationOffset = 0;
    errorCode = memoryManager.allocate(thirdAllocationSize, thirdAllocationOffset);
    retail_assert(errorCode == success, "Third allocation has failed!");
    output_allocation_information(thirdAllocationSize, thirdAllocationOffset);

    retail_assert(
        thirdAllocationOffset == secondAllocationOffset + secondAllocationSize + sizeof(memory_allocation_metadata), 
        "Third allocation offset is not the expected value.");

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** MemoryManager basic operation tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}

void test_memory_manager_advanced_operation()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** MemoryManager advanced operation tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    const size_t memorySize = 8000;
    const size_t mainMemorySystemReservedSize = 1000;
    uint8_t memory[memorySize];

    memory_manager memoryManager;

    execution_flags executionFlags;

    gaia::db::memory_manager::error_code errorCode = not_set;

    executionFlags.enable_extra_validations = true;
    executionFlags.enable_console_output = true;

    memoryManager.set_execution_flags(executionFlags);
    errorCode = memoryManager.manage(memory, memorySize, mainMemorySystemReservedSize, true);
    retail_assert(errorCode == success, "Manager initialization has failed!");
    cout << "PASSED: Manager initialization was successful!" << endl;

    size_t stackAllocatorMemorySize = 2000;

    // Make 3 allocations using a StackAllocator.
    stack_allocator* pStackAllocator = nullptr;
    errorCode = memoryManager.create_stack_allocator(stackAllocatorMemorySize, pStackAllocator);
    retail_assert(errorCode == success, "First stack allocator creation has failed!");

    size_t firstAllocationSize = 64;
    size_t secondAllocationSize = 256;
    size_t thirdAllocationSize = 128;

    ADDRESS_OFFSET firstAllocationOffset = 0;
    errorCode = pStackAllocator->allocate(0, 0, firstAllocationSize, firstAllocationOffset);
    retail_assert(errorCode == success, "First allocation has failed!");
    output_allocation_information(firstAllocationSize, firstAllocationOffset);

    ADDRESS_OFFSET secondAllocationOffset = 0;
    errorCode = pStackAllocator->allocate(0, 0, secondAllocationSize, secondAllocationOffset);
    retail_assert(errorCode == success, "Second allocation has failed!");
    output_allocation_information(secondAllocationSize, secondAllocationOffset);

    retail_assert(
        secondAllocationOffset == firstAllocationOffset + firstAllocationSize + sizeof(memory_allocation_metadata),
        "Second allocation offset does not have expected value.");

    ADDRESS_OFFSET thirdAllocationOffset = 0;
    errorCode = pStackAllocator->allocate(0, 0, thirdAllocationSize, thirdAllocationOffset);
    retail_assert(errorCode == success, "Third allocation has failed!");
    output_allocation_information(thirdAllocationSize, thirdAllocationOffset);

    retail_assert(
        thirdAllocationOffset == secondAllocationOffset + secondAllocationSize + sizeof(memory_allocation_metadata),
        "Third allocation offset does not have expected value.");

    retail_assert(pStackAllocator->get_allocation_count() == 3, "Allocation count is not the expected 3!");

    // Commit stack allocator.
    size_t serializationNumber = 331;
    cout << endl << "Commit first stack allocator with serialization number " << serializationNumber << "..." << endl;
    errorCode = memoryManager.commit_stack_allocator(pStackAllocator, serializationNumber);
    retail_assert(errorCode == success, "First sta>ck allocator commit has failed!");

    memory_list_node* pFirstReadListHead = nullptr;
    errorCode = memoryManager.get_unserialized_allocations_list_head(pFirstReadListHead);
    retail_assert(errorCode == success, "Failed first call to get unserialized allocations list head!");
    retail_assert(pFirstReadListHead != nullptr, "Failed the first attempt to retrieve a valid unserialized allocations list head.");

    // Make 2 more allocations using a new StackAllocator.
    // Both allocations will replace earlier allocations (4th replaces 2nd and 5th replaces 1st),
    // which will get garbage collected at commit time.
    errorCode = memoryManager.create_stack_allocator(stackAllocatorMemorySize, pStackAllocator);
    retail_assert(errorCode == success, "Second stack allocator creation has failed!");

    size_t fourthAllocationSize = 256;
    size_t fifthAllocationSize = 64;

    ADDRESS_OFFSET fourthAllocationOffset = 0;
    errorCode = pStackAllocator->allocate(0, secondAllocationOffset, fourthAllocationSize, fourthAllocationOffset);
    retail_assert(errorCode == success, "Fourth allocation has failed!");
    output_allocation_information(fourthAllocationSize, fourthAllocationOffset);

    ADDRESS_OFFSET fifthAllocationOffset = 0;
    errorCode = pStackAllocator->allocate(0, firstAllocationOffset, fifthAllocationSize, fifthAllocationOffset);
    retail_assert(errorCode == success, "Fifth allocation has failed!");
    output_allocation_information(fifthAllocationSize, fifthAllocationOffset);

    retail_assert(
        fourthAllocationOffset + fourthAllocationSize + sizeof(memory_allocation_metadata) == fifthAllocationOffset,
        "Second allocation offset does not have expected value.");

    // Commit stack allocator.
    serializationNumber = 332;
    cout << endl << "Commit second stack allocator with serialization number " << serializationNumber << "..." << endl;
    errorCode = memoryManager.commit_stack_allocator(pStackAllocator, serializationNumber);
    retail_assert(errorCode == success, "Second stack allocator commit has failed!");

    memory_list_node* pSecondReadListHead = nullptr;
    errorCode = memoryManager.get_unserialized_allocations_list_head(pSecondReadListHead);
    retail_assert(errorCode == success, "Failed second call to get unserialized allocations list head!");
    retail_assert(pSecondReadListHead != nullptr, "Failed the second attempt to retrieve a valid unserialized allocations list head.");

    retail_assert(
        pSecondReadListHead->next == pFirstReadListHead->next,
        "First node in unserialized allocations list should not have changed!");

    // Validate content of unserialized allocations list.
    retail_assert(
        pSecondReadListHead->next != 0,
        "The unserialized allocations list should contain a first entry!");
    memory_record* pFirstUnserializedRecord = memoryManager.read_memory_record(pSecondReadListHead->next);
    retail_assert(
        pFirstUnserializedRecord->next != 0,
        "The unserialized allocations list should contain a second entry!");
    ADDRESS_OFFSET secondUnserializedRecordOffset = pFirstUnserializedRecord->next;

    // Update unserialized allocations list.
    cout << endl << "Calling UpdateUnserializedAllocationsListHead() with parameter: ";
    cout << secondUnserializedRecordOffset << "." << endl;
    errorCode = memoryManager.update_unserialized_allocations_list_head(pFirstUnserializedRecord->next);
    retail_assert(errorCode == success, "Failed first call to UpdateUnserializedAllocationsListHead()!");

    memory_list_node* pThirdReadListHead = nullptr;
    errorCode = memoryManager.get_unserialized_allocations_list_head(pThirdReadListHead);
    retail_assert(errorCode == success, "Failed third call to get unserialized allocations list head!");
    retail_assert(pThirdReadListHead != nullptr, "Failed the third attempt to retrieve a valid unserialized allocations list head.");

    retail_assert(
        pThirdReadListHead->next == secondUnserializedRecordOffset,
        "The value of the first node of the unserialized allocations list is not the expected one!");

    // Test allocating from freed memory.
    // First, we reclaim a full freed block.
    ADDRESS_OFFSET offsetFirstFreeAllocation = 0;
    errorCode = memoryManager.allocate(secondAllocationSize, offsetFirstFreeAllocation);
    retail_assert(errorCode == success, "First free allocation has failed!");
    output_allocation_information(secondAllocationSize, offsetFirstFreeAllocation);

    // Second, we reclaim a part of a freed block.
    ADDRESS_OFFSET offsetSecondFreeAllocation = 0;
    errorCode = memoryManager.allocate(thirdAllocationSize, offsetSecondFreeAllocation);
    retail_assert(errorCode == success, "First free allocation has failed!");
    output_allocation_information(thirdAllocationSize, offsetSecondFreeAllocation);

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** MemoryManager advanced operation tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
