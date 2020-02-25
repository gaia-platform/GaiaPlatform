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

void output_struct_sizes();
void test_pointer_arithmetic();

int main()
{
    output_struct_sizes();
    test_pointer_arithmetic();

    cout << endl << c_all_tests_passed << endl;
}

void output_struct_sizes()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** StructSizes tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    cout << "sizeof(MemoryAllocationMetadata) = " << sizeof(memory_allocation_metadata) << endl;
    cout << "sizeof(StackAllocatorMetadata) = " << sizeof(stack_allocator_metadata) << endl;
    cout << "sizeof(StackAllocatorAllocation) = " << sizeof(stack_allocator_allocation) << endl;

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** StructSizes tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}

void test_pointer_arithmetic()
{
    struct small_integers
    {
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
    };

    struct integers
    {
        small_integers small_integers;
        uint64_t uint64;
    };

    integers integers;

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** PointerArithmetic tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    cout << "address(integers) = " << &integers << " = " << (size_t)&integers << endl;
    cout << "address(uint8) = " << &integers.small_integers.uint8
        << " = " << (size_t)&integers.small_integers.uint8 << endl;
    cout << "address(uint16) = " << &integers.small_integers.uint16
        << " = " << (size_t)&integers.small_integers.uint16 << endl;
    cout << "address(uint32) = " << &integers.small_integers.uint32
        << " = " << (size_t)&integers.small_integers.uint32 << endl;
    cout << "address(uint64) = " << &integers.uint64 << " = " << (size_t)&integers.uint64 << endl;

    cout << endl << "sizeof(SmallIntegers) = " << sizeof(small_integers) << endl;
    cout << "&integers.uint64 - &integers = " << (uint8_t*)&integers.uint64 - (uint8_t*)&integers << endl;
    cout << "&integers.uint64 - &integers.smallIntegers.uint8 = "
        << (uint8_t*)&integers.uint64 - (uint8_t*)&integers.small_integers.uint8 << endl;

    retail_assert(
        (uint8_t*)&integers.uint64 - (uint8_t*)&integers == sizeof(small_integers),
        "Pointer difference didn't result in structure size!");
    retail_assert(
        (uint8_t*)&integers + sizeof(small_integers) == (uint8_t*)&integers.uint64,
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
