/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>

#include <gtest/gtest.h>

#include "base_memory_manager.hpp"
#include "memory_structures.hpp"

using namespace std;

using namespace gaia::db::memory_manager;

TEST(memory_manager, struct_sizes)
{
    cout << "Executing function " << __func__ << ".\n"
         << endl;
    cout << "sizeof(memory_manager_metadata_t) = " << sizeof(memory_manager_metadata_t) << endl;
    cout << "sizeof(chunk_manager_metadata_t) = " << sizeof(chunk_manager_metadata_t) << endl;
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

    cout << "address(integers) = " << &integers << " = " << reinterpret_cast<size_t>(&integers) << endl;
    cout << "address(uint8) = " << &integers.small_integers.uint8
         << " = " << reinterpret_cast<size_t>(&integers.small_integers.uint8) << endl;
    cout << "address(uint16) = " << &integers.small_integers.uint16
         << " = " << reinterpret_cast<size_t>(&integers.small_integers.uint16) << endl;
    cout << "address(uint32) = " << &integers.small_integers.uint32
         << " = " << reinterpret_cast<size_t>(&integers.small_integers.uint32) << endl;
    cout << "address(uint64) = " << &integers.uint64 << " = " << reinterpret_cast<size_t>(&integers.uint64) << endl;

    cout << endl
         << "sizeof(small_integers_t) = " << sizeof(small_integers_t) << endl;
    cout << "&integers.uint64 - &integers = "
         << reinterpret_cast<uint8_t*>(&integers.uint64) - reinterpret_cast<uint8_t*>(&integers) << endl;
    cout << "&integers.uint64 - &integers.small_integers.uint8 = "
         << reinterpret_cast<uint8_t*>(&integers.uint64) - reinterpret_cast<uint8_t*>(&integers.small_integers.uint8)
         << endl;

    ASSERT_EQ(
        sizeof(small_integers_t),
        reinterpret_cast<uint8_t*>(&integers.uint64) - reinterpret_cast<uint8_t*>(&integers));
    ASSERT_EQ(
        reinterpret_cast<uint8_t*>(&integers.uint64),
        reinterpret_cast<uint8_t*>(&integers) + sizeof(small_integers_t));

    constexpr size_t c_starting_value = -8;
    constexpr size_t c_additional_value = 20;
    constexpr size_t c_expected_result = 12;

    cout << endl
         << "Test integer overflow:" << endl;
    size_t value = c_starting_value;
    cout << "Large value, 8 away from overflow: " << value;
    value += c_additional_value;
    cout << " + " << c_additional_value << " = " << value << endl;

    ASSERT_EQ(c_expected_result, value);
}
