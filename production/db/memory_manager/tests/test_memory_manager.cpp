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

void TestMemoryManagerBasicOperation();
void TestMemoryManagerAdvancedOperation();

int main()
{
    TestMemoryManagerBasicOperation();

    TestMemoryManagerAdvancedOperation();

    cout << endl << c_all_tests_passed << endl;
}

void OutputAllocationInformation(size_t size, ADDRESS_OFFSET offset)
{
    cout << endl << size << " bytes were allocated at offset " << offset << "." << endl;
}

void TestMemoryManagerBasicOperation()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** MemoryManager basic operation tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    const size_t memorySize = 8000;
    const size_t mainMemorySystemReservedSize = 1000;
    uint8_t memory[memorySize];

    CMemoryManager memoryManager;

    ExecutionFlags executionFlags;

    EMemoryManagerErrorCode errorCode = mmec_NotSet;

    executionFlags.enableExtraValidations = true;
    executionFlags.enableConsoleOutput = true;

    memoryManager.SetExecutionFlags(executionFlags);
    errorCode = memoryManager.Manage(memory, memorySize, mainMemorySystemReservedSize, true);
    retail_assert(errorCode == mmec_Success, "Manager initialization has failed!");
    cout << "PASSED: Manager initialization was successful!" << endl;

    size_t firstAllocationSize = 64;
    size_t secondAllocationSize = 256;
    size_t thirdAllocationSize = 128;

    ADDRESS_OFFSET firstAllocationOffset = 0;
    errorCode = memoryManager.Allocate(firstAllocationSize, firstAllocationOffset);
    retail_assert(errorCode == mmec_Success, "First allocation has failed!");
    OutputAllocationInformation(firstAllocationSize, firstAllocationOffset);

    ADDRESS_OFFSET secondAllocationOffset = 0;
    errorCode = memoryManager.Allocate(secondAllocationSize, secondAllocationOffset);
    retail_assert(errorCode == mmec_Success, "Second allocation has failed!");
    OutputAllocationInformation(secondAllocationSize, secondAllocationOffset);

    retail_assert(
        secondAllocationOffset == firstAllocationOffset + firstAllocationSize + sizeof(MemoryAllocationMetadata), 
        "Second allocation offset is not the expected value.");

    ADDRESS_OFFSET thirdAllocationOffset = 0;
    errorCode = memoryManager.Allocate(thirdAllocationSize, thirdAllocationOffset);
    retail_assert(errorCode == mmec_Success, "Third allocation has failed!");
    OutputAllocationInformation(thirdAllocationSize, thirdAllocationOffset);

    retail_assert(
        thirdAllocationOffset == secondAllocationOffset + secondAllocationSize + sizeof(MemoryAllocationMetadata), 
        "Third allocation offset is not the expected value.");

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** MemoryManager basic operation tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}

void TestMemoryManagerAdvancedOperation()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** MemoryManager advanced operation tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    const size_t memorySize = 8000;
    const size_t mainMemorySystemReservedSize = 1000;
    uint8_t memory[memorySize];

    CMemoryManager memoryManager;

    ExecutionFlags executionFlags;

    EMemoryManagerErrorCode errorCode = mmec_NotSet;

    executionFlags.enableExtraValidations = true;
    executionFlags.enableConsoleOutput = true;

    memoryManager.SetExecutionFlags(executionFlags);
    errorCode = memoryManager.Manage(memory, memorySize, mainMemorySystemReservedSize, true);
    retail_assert(errorCode == mmec_Success, "Manager initialization has failed!");
    cout << "PASSED: Manager initialization was successful!" << endl;

    size_t stackAllocatorMemorySize = 2000;

    // Make 3 allocations using a StackAllocator.
    CStackAllocator* pStackAllocator = nullptr;
    errorCode = memoryManager.CreateStackAllocator(stackAllocatorMemorySize, pStackAllocator);
    retail_assert(errorCode == mmec_Success, "First stack allocator creation has failed!");

    size_t firstAllocationSize = 64;
    size_t secondAllocationSize = 256;
    size_t thirdAllocationSize = 128;

    ADDRESS_OFFSET firstAllocationOffset = 0;
    errorCode = pStackAllocator->Allocate(0, 0, firstAllocationSize, firstAllocationOffset);
    retail_assert(errorCode == mmec_Success, "First allocation has failed!");
    OutputAllocationInformation(firstAllocationSize, firstAllocationOffset);

    ADDRESS_OFFSET secondAllocationOffset = 0;
    errorCode = pStackAllocator->Allocate(0, 0, secondAllocationSize, secondAllocationOffset);
    retail_assert(errorCode == mmec_Success, "Second allocation has failed!");
    OutputAllocationInformation(secondAllocationSize, secondAllocationOffset);

    retail_assert(
        secondAllocationOffset == firstAllocationOffset + firstAllocationSize + sizeof(MemoryAllocationMetadata),
        "Second allocation offset does not have expected value.");

    ADDRESS_OFFSET thirdAllocationOffset = 0;
    errorCode = pStackAllocator->Allocate(0, 0, thirdAllocationSize, thirdAllocationOffset);
    retail_assert(errorCode == mmec_Success, "Third allocation has failed!");
    OutputAllocationInformation(thirdAllocationSize, thirdAllocationOffset);

    retail_assert(
        thirdAllocationOffset == secondAllocationOffset + secondAllocationSize + sizeof(MemoryAllocationMetadata),
        "Third allocation offset does not have expected value.");

    retail_assert(pStackAllocator->GetAllocationCount() == 3, "Allocation count is not the expected 3!");

    // Commit stack allocator.
    size_t serializationNumber = 331;
    cout << endl << "Commit first stack allocator with serialization number " << serializationNumber << "..." << endl;
    errorCode = memoryManager.CommitStackAllocator(pStackAllocator, serializationNumber);
    retail_assert(errorCode == mmec_Success, "First sta>ck allocator commit has failed!");

    MemoryListNode* pFirstReadListHead = nullptr;
    errorCode = memoryManager.GetUnserializedAllocationsListHead(pFirstReadListHead);
    retail_assert(errorCode == mmec_Success, "Failed first call to get unserialized allocations list head!");
    retail_assert(pFirstReadListHead != nullptr, "Failed the first attempt to retrieve a valid unserialized allocations list head.");

    // Make 2 more allocations using a new StackAllocator.
    // Both allocations will replace earlier allocations (4th replaces 2nd and 5th replaces 1st),
    // which will get garbage collected at commit time.
    errorCode = memoryManager.CreateStackAllocator(stackAllocatorMemorySize, pStackAllocator);
    retail_assert(errorCode == mmec_Success, "Second stack allocator creation has failed!");

    size_t fourthAllocationSize = 256;
    size_t fifthAllocationSize = 64;

    ADDRESS_OFFSET fourthAllocationOffset = 0;
    errorCode = pStackAllocator->Allocate(0, secondAllocationOffset, fourthAllocationSize, fourthAllocationOffset);
    retail_assert(errorCode == mmec_Success, "Fourth allocation has failed!");
    OutputAllocationInformation(fourthAllocationSize, fourthAllocationOffset);

    ADDRESS_OFFSET fifthAllocationOffset = 0;
    errorCode = pStackAllocator->Allocate(0, firstAllocationOffset, fifthAllocationSize, fifthAllocationOffset);
    retail_assert(errorCode == mmec_Success, "Fifth allocation has failed!");
    OutputAllocationInformation(fifthAllocationSize, fifthAllocationOffset);

    retail_assert(
        fourthAllocationOffset + fourthAllocationSize + sizeof(MemoryAllocationMetadata) == fifthAllocationOffset,
        "Second allocation offset does not have expected value.");

    // Commit stack allocator.
    serializationNumber = 332;
    cout << endl << "Commit second stack allocator with serialization number " << serializationNumber << "..." << endl;
    errorCode = memoryManager.CommitStackAllocator(pStackAllocator, serializationNumber);
    retail_assert(errorCode == mmec_Success, "Second stack allocator commit has failed!");

    MemoryListNode* pSecondReadListHead = nullptr;
    errorCode = memoryManager.GetUnserializedAllocationsListHead(pSecondReadListHead);
    retail_assert(errorCode == mmec_Success, "Failed second call to get unserialized allocations list head!");
    retail_assert(pSecondReadListHead != nullptr, "Failed the second attempt to retrieve a valid unserialized allocations list head.");

    retail_assert(
        pSecondReadListHead->next == pFirstReadListHead->next,
        "First node in unserialized allocations list should not have changed!");

    // Validate content of unserialized allocations list.
    retail_assert(
        pSecondReadListHead->next != 0,
        "The unserialized allocations list should contain a first entry!");
    MemoryRecord* pFirstUnserializedRecord = memoryManager.ReadMemoryRecord(pSecondReadListHead->next);
    retail_assert(
        pFirstUnserializedRecord->next != 0,
        "The unserialized allocations list should contain a second entry!");
    ADDRESS_OFFSET secondUnserializedRecordOffset = pFirstUnserializedRecord->next;

    // Update unserialized allocations list.
    cout << endl << "Calling UpdateUnserializedAllocationsListHead() with parameter: ";
    cout << secondUnserializedRecordOffset << "." << endl;
    errorCode = memoryManager.UpdateUnserializedAllocationsListHead(pFirstUnserializedRecord->next);
    retail_assert(errorCode == mmec_Success, "Failed first call to UpdateUnserializedAllocationsListHead()!");

    MemoryListNode* pThirdReadListHead = nullptr;
    errorCode = memoryManager.GetUnserializedAllocationsListHead(pThirdReadListHead);
    retail_assert(errorCode == mmec_Success, "Failed third call to get unserialized allocations list head!");
    retail_assert(pThirdReadListHead != nullptr, "Failed the third attempt to retrieve a valid unserialized allocations list head.");

    retail_assert(
        pThirdReadListHead->next == secondUnserializedRecordOffset,
        "The value of the first node of the unserialized allocations list is not the expected one!");

    // Test allocating from freed memory.
    // First, we reclaim a full freed block.
    ADDRESS_OFFSET offsetFirstFreeAllocation = 0;
    errorCode = memoryManager.Allocate(secondAllocationSize, offsetFirstFreeAllocation);
    retail_assert(errorCode == mmec_Success, "First free allocation has failed!");
    OutputAllocationInformation(secondAllocationSize, offsetFirstFreeAllocation);

    // Second, we reclaim a part of a freed block.
    ADDRESS_OFFSET offsetSecondFreeAllocation = 0;
    errorCode = memoryManager.Allocate(thirdAllocationSize, offsetSecondFreeAllocation);
    retail_assert(errorCode == mmec_Success, "First free allocation has failed!");
    OutputAllocationInformation(thirdAllocationSize, offsetSecondFreeAllocation);

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** MemoryManager advanced operation tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
