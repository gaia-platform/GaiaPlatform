/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>

#include "constants.hpp"
#include "retail_assert.hpp"
#include "memory_structures.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db::memory_manager;

void OutputStructSizes();
void TestPointerArithmetic();

int main()
{
    OutputStructSizes();
    TestPointerArithmetic();

    cout << endl << c_all_tests_passed << endl;
}

void OutputStructSizes()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** StructSizes tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    cout << "sizeof(MemoryAllocationMetadata) = " << sizeof(MemoryAllocationMetadata) << endl;
    cout << "sizeof(StackAllocatorMetadata) = " << sizeof(StackAllocatorMetadata) << endl;
    cout << "sizeof(StackAllocatorAllocation) = " << sizeof(StackAllocatorAllocation) << endl;

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** StructSizes tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}

void TestPointerArithmetic()
{
    struct SmallIntegers
    {
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
    };

    struct Integers
    {
        SmallIntegers smallIntegers;
        uint64_t uint64;
    };

    Integers integers;

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** PointerArithmetic tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    cout << "address(integers) = " << &integers << " = " << (size_t)&integers << endl;
    cout << "address(uint8) = " << &integers.smallIntegers.uint8
        << " = " << (size_t)&integers.smallIntegers.uint8 << endl;
    cout << "address(uint16) = " << &integers.smallIntegers.uint16
        << " = " << (size_t)&integers.smallIntegers.uint16 << endl;
    cout << "address(uint32) = " << &integers.smallIntegers.uint32
        << " = " << (size_t)&integers.smallIntegers.uint32 << endl;
    cout << "address(uint64) = " << &integers.uint64 << " = " << (size_t)&integers.uint64 << endl;

    cout << endl << "sizeof(SmallIntegers) = " << sizeof(SmallIntegers) << endl;
    cout << "&integers.uint64 - &integers = " << (uint8_t*)&integers.uint64 - (uint8_t*)&integers << endl;
    cout << "&integers.uint64 - &integers.smallIntegers.uint8 = "
        << (uint8_t*)&integers.uint64 - (uint8_t*)&integers.smallIntegers.uint8 << endl;

    retail_assert(
        (uint8_t*)&integers.uint64 - (uint8_t*)&integers == sizeof(SmallIntegers),
        "Pointer difference didn't result in structure size!");
    retail_assert(
        (uint8_t*)&integers + sizeof(SmallIntegers) == (uint8_t*)&integers.uint64,
        "Pointer addition didn't result in field address!");

    cout << endl << "Test integer overflow:" << endl;
    size_t value = -8;
    cout << "Large value, 8 away from overflow: " << value;
    value += 20;
    cout << " + 20 = " << value << endl;

    retail_assert(
        value == 12,
        "Addition did not overflow!");

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** PointerArithmetic tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
