/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>

#include "gtest/gtest.h"

#include "base_memory_manager.hpp"
#include "memory_structures.hpp"

using namespace std;

using namespace gaia::db::memory_manager;

TEST(memory_manager, struct_sizes)
{
    cout << "sizeof(memory_allocation_metadata_t) = " << sizeof(memory_allocation_metadata_t) << endl;
    cout << "sizeof(stack_allocator_metadata_t) = " << sizeof(stack_allocator_metadata_t) << endl;
    cout << "sizeof(stack_allocator_allocation_t) = " << sizeof(stack_allocator_allocation_t) << endl;
}

TEST(system, pointer_arithmetic)
{
    struct small_integers_t
    {
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
    };

    struct integers_t
    {
        small_integers_t small_integers;
        uint64_t uint64;
    };

    integers_t integers;

    cout << "address(integers) = " << &integers << " = " << (size_t)&integers << endl;
    cout << "address(uint8) = " << &integers.small_integers.uint8
        << " = " << (size_t)&integers.small_integers.uint8 << endl;
    cout << "address(uint16) = " << &integers.small_integers.uint16
        << " = " << (size_t)&integers.small_integers.uint16 << endl;
    cout << "address(uint32) = " << &integers.small_integers.uint32
        << " = " << (size_t)&integers.small_integers.uint32 << endl;
    cout << "address(uint64) = " << &integers.uint64 << " = " << (size_t)&integers.uint64 << endl;

    cout << endl << "sizeof(small_integers_t) = " << sizeof(small_integers_t) << endl;
    cout << "&integers.uint64 - &integers = " << (uint8_t*)&integers.uint64 - (uint8_t*)&integers << endl;
    cout << "&integers.uint64 - &integers.small_integers.uint8 = "
        << (uint8_t*)&integers.uint64 - (uint8_t*)&integers.small_integers.uint8 << endl;

    ASSERT_EQ(sizeof(small_integers_t), (uint8_t*)&integers.uint64 - (uint8_t*)&integers);
    ASSERT_EQ((uint8_t*)&integers.uint64, (uint8_t*)&integers + sizeof(small_integers_t));

    cout << endl << "Test integer overflow:" << endl;
    size_t value = -8;
    cout << "Large value, 8 away from overflow: " << value;
    value += 20;
    cout << " + 20 = " << value << endl;

    ASSERT_EQ(12, value);
}
