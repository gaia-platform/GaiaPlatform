/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "bitmap.hpp"

using namespace std;

using namespace gaia::db::memory_manager;

TEST(bitmap, bit_setting)
{
    uint64_t bitmap = 0;

    set_bit_value(&bitmap, 1, 2, true);
    print_bitmap(&bitmap, 1);
    ASSERT_EQ(bitmap, 4);

    uint64_t bit_count = count_set_bits(&bitmap, 1);
    ASSERT_EQ(bit_count, 1);

    uint64_t bit_index = find_first_unset_bit(&bitmap, 1);
    ASSERT_EQ(bit_index, 0);

    set_bit_value(&bitmap, 1, 2, false);
    print_bitmap(&bitmap, 1);
    ASSERT_EQ(bitmap, 0);

    bit_count = count_set_bits(&bitmap, 1);
    ASSERT_EQ(bit_count, 0);

    set_bit_value(&bitmap, 1, 10, true);
    print_bitmap(&bitmap, 1);
    ASSERT_EQ(bitmap, 1024);

    set_bit_value(&bitmap, 1, 11, true);
    print_bitmap(&bitmap, 1);
    ASSERT_EQ(bitmap, 3072);

    set_bit_value(&bitmap, 1, 12, true);
    print_bitmap(&bitmap, 1);
    ASSERT_EQ(bitmap, 7168);

    bit_count = count_set_bits(&bitmap, 1);
    ASSERT_EQ(bit_count, 3);

    set_bit_value(&bitmap, 1, 11, false);
    print_bitmap(&bitmap, 1);
    ASSERT_EQ(bitmap, 5120);

    bit_count = count_set_bits(&bitmap, 1);
    ASSERT_EQ(bit_count, 2);
}

TEST(bitmap, bit_range_setting)
{
    constexpr uint64_t c_bitmap_size = 5;
    uint64_t bitmap[c_bitmap_size] = {0};

    set_bit_range_value(bitmap, c_bitmap_size, 3, 3, true);
    print_bitmap(bitmap, 1);
    ASSERT_EQ(bitmap[0], 56);

    uint64_t bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 3);

    uint64_t bit_index = find_first_unset_bit(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_index, 0);

    set_bit_range_value(bitmap, c_bitmap_size, 2, 5, false);
    print_bitmap(bitmap, 1);
    ASSERT_EQ(bitmap[0], 0);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 0);

    set_bit_range_value(bitmap, c_bitmap_size, 10, 65, true);
    print_bitmap(bitmap, c_bitmap_size);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 65);

    bit_index = find_first_unset_bit(&bitmap[1], 1);
    ASSERT_EQ(bit_index, 11);

    set_bit_range_value(bitmap, c_bitmap_size, 11, 63, false);
    print_bitmap(bitmap, c_bitmap_size);
    ASSERT_EQ(bitmap[0], 1024);
    ASSERT_EQ(bitmap[1], 1024);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 2);

    set_bit_range_value(bitmap, c_bitmap_size, 138, 129, true);
    print_bitmap(bitmap, c_bitmap_size);
    ASSERT_EQ(bitmap[3], -1);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 131);

    bit_index = find_first_unset_bit(&bitmap[3], 2);
    ASSERT_EQ(bit_index, 75);

    set_bit_range_value(bitmap, c_bitmap_size, 139, 127, false);
    print_bitmap(bitmap, c_bitmap_size);
    ASSERT_EQ(bitmap[2], 1024);
    ASSERT_EQ(bitmap[3], 0);
    ASSERT_EQ(bitmap[4], 1024);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 4);
}
