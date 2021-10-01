/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <atomic>
#include <iostream>

#include <gtest/gtest.h>

#include "bitmap.hpp"

using namespace std;

using namespace gaia::db::memory_manager;

TEST(bitmap, set_bit_value_and_is_bit_set)
{
    constexpr size_t c_bitmap_size = 3;
    std::atomic<uint64_t> bitmap[c_bitmap_size];

    // Start with an empty bitmap.
    // Set each bit of the bitmap and verify that it was set and that only one bit was set.
    bitmap[0] = bitmap[1] = bitmap[2] = 0;
    for (size_t i = 0; i < c_bitmap_size * c_uint64_bit_count; ++i)
    {
        set_bit_value(bitmap, c_bitmap_size, i, true);
        ASSERT_EQ(true, is_bit_set(bitmap, c_bitmap_size, i));
        ASSERT_EQ(1, count_set_bits(bitmap, c_bitmap_size));
        bitmap[i / c_uint64_bit_count] = 0;
    }

    // Start with a full bitmap.
    // Unset each bit of the bitmap and verify that it was unset and that only one bit was unset.
    bitmap[0] = bitmap[1] = bitmap[2] = -1;
    for (size_t i = 0; i < c_bitmap_size * c_uint64_bit_count; ++i)
    {
        set_bit_value(bitmap, c_bitmap_size, i, false);
        ASSERT_EQ(false, is_bit_set(bitmap, c_bitmap_size, i));
        ASSERT_EQ(c_bitmap_size * c_uint64_bit_count - 1, count_set_bits(bitmap, c_bitmap_size));
        bitmap[i / c_uint64_bit_count] = -1;
    }
}

TEST(bitmap, find_first_unset_bit)
{
    constexpr size_t c_bitmap_size = 3;
    std::atomic<uint64_t> bitmap[c_bitmap_size];

    // Start with an empty bitmap.
    // Keep setting bits in the first two words of the bitmap
    // and verify that find_first_unset_bit finds the next unset bit.
    bitmap[0] = bitmap[1] = bitmap[2] = 0;
    for (size_t i = 0; i < (c_bitmap_size - 1) * c_uint64_bit_count; ++i)
    {
        size_t bit_index_in_word = i % c_uint64_bit_count;
        if (bit_index_in_word != 0)
        {
            bitmap[i / c_uint64_bit_count] = (1ULL << bit_index_in_word) - 1;
        }

        ASSERT_EQ(i, find_first_unset_bit(bitmap, c_bitmap_size));

        // After we're done with a word, leave all bits set,
        // so the search will go into the next word.
        if (bit_index_in_word == c_uint64_bit_count - 1)
        {
            bitmap[i / c_uint64_bit_count] = -1;
        }
    }

    // Check that we have set the bits of the first 2 words of the bitmap.
    ASSERT_EQ((c_bitmap_size - 1) * c_uint64_bit_count, count_set_bits(bitmap, c_bitmap_size));
}

TEST(bitmap, limit)
{
    std::atomic<uint64_t> bitmap = 0;
    size_t end_limit_bit_index = 7;

    // Start with an empty bitmap.
    // and verify that find_first_unset_bit finds the next unset bit.
    for (size_t i = 0; i <= c_uint64_bit_count; ++i)
    {
        if (i == c_uint64_bit_count)
        {
            bitmap = -1;
        }
        else
        {
            bitmap = (1ULL << i) - 1;
        }

        if (i <= end_limit_bit_index)
        {
            ASSERT_EQ(i, find_first_unset_bit(&bitmap, 1, end_limit_bit_index));
        }
        else
        {
            ASSERT_EQ(c_max_bit_index, find_first_unset_bit(&bitmap, 1, end_limit_bit_index));
        }

        if (i <= end_limit_bit_index)
        {
            ASSERT_EQ(i, count_set_bits(&bitmap, 1, end_limit_bit_index));
        }
        else
        {
            ASSERT_EQ(end_limit_bit_index + 1, count_set_bits(&bitmap, 1, end_limit_bit_index));
        }
    }

    // Check that we have set the bits of the bitmap.
    ASSERT_EQ(c_uint64_bit_count, count_set_bits(&bitmap, 1));
}

TEST(bitmap, count_set_bits)
{
    constexpr size_t c_bitmap_size = 3;
    std::atomic<uint64_t> bitmap[c_bitmap_size];

    // Start with an empty bitmap.
    // Keep setting bits in the first two words of the bitmap
    // and verify that count_set_bits counts them properly.
    bitmap[0] = bitmap[1] = bitmap[2] = 0;
    for (size_t i = 0; i < (c_bitmap_size - 1) * c_uint64_bit_count; ++i)
    {
        size_t bit_index_in_word = i % c_uint64_bit_count;
        if (bit_index_in_word != 0)
        {
            bitmap[i / c_uint64_bit_count] = (1ULL << bit_index_in_word) - 1;
        }

        ASSERT_EQ(i, count_set_bits(bitmap, c_bitmap_size));

        if (bit_index_in_word == c_uint64_bit_count - 1)
        {
            bitmap[i / c_uint64_bit_count] = -1;
        }
    }

    // Check that we have set the bits of the first 2 words of the bitmap.
    ASSERT_EQ((c_bitmap_size - 1) * c_uint64_bit_count, count_set_bits(bitmap, c_bitmap_size));
}

TEST(bitmap, bit_setting)
{
    std::atomic<uint64_t> bitmap = 0;

    set_bit_value(&bitmap, 1, 2, true);
    print_bitmap(&bitmap, 1);
    ASSERT_EQ(bitmap, 4);

    size_t bit_count = count_set_bits(&bitmap, 1);
    ASSERT_EQ(bit_count, 1);

    size_t bit_index = find_first_unset_bit(&bitmap, 1);
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
    constexpr size_t c_bitmap_size = 5;
    std::atomic<uint64_t> bitmap[c_bitmap_size]{0};

    safe_set_bit_range_value(bitmap, c_bitmap_size, 3, 3, true);
    print_bitmap(bitmap, 1);
    ASSERT_EQ(bitmap[0], 56);

    size_t bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 3);

    size_t bit_index = find_first_unset_bit(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_index, 0);

    safe_set_bit_range_value(bitmap, c_bitmap_size, 2, 5, false);
    print_bitmap(bitmap, 1);
    ASSERT_EQ(bitmap[0], 0);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 0);

    safe_set_bit_range_value(bitmap, c_bitmap_size, 10, 65, true);
    print_bitmap(bitmap, c_bitmap_size);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 65);

    bit_index = find_first_unset_bit(&bitmap[1], 1);
    ASSERT_EQ(bit_index, 11);

    safe_set_bit_range_value(bitmap, c_bitmap_size, 11, 63, false);
    print_bitmap(bitmap, c_bitmap_size);
    ASSERT_EQ(bitmap[0], 1024);
    ASSERT_EQ(bitmap[1], 1024);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 2);

    safe_set_bit_range_value(bitmap, c_bitmap_size, 138, 129, true);
    print_bitmap(bitmap, c_bitmap_size);
    ASSERT_EQ(bitmap[3], -1);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 131);

    bit_index = find_first_unset_bit(&bitmap[3], 2);
    ASSERT_EQ(bit_index, 75);

    safe_set_bit_range_value(bitmap, c_bitmap_size, 139, 127, false);
    print_bitmap(bitmap, c_bitmap_size);
    ASSERT_EQ(bitmap[2], 1024);
    ASSERT_EQ(bitmap[3], 0);
    ASSERT_EQ(bitmap[4], 1024);

    bit_count = count_set_bits(bitmap, c_bitmap_size);
    ASSERT_EQ(bit_count, 4);
}

TEST(bitarray, set_get_find_element)
{
    constexpr size_t c_bitarray_size = 2;
    std::atomic<uint64_t> bitarray[c_bitarray_size]{};

    constexpr uint64_t c_element_value = 0b1101;
    constexpr size_t c_element_width = 4;
    constexpr size_t c_bitarray_index = 17;

    set_element_at_index(bitarray, c_bitarray_size, c_element_width, c_bitarray_index, c_element_value);

    // We need the same bit ordering as the binary literal representation.
    bool msb_first = true;
    print_bitmap(bitarray, c_bitarray_size, msb_first);

    ASSERT_EQ(bitarray[0], 0);
    ASSERT_EQ(bitarray[1], c_element_value << c_element_width);

    size_t bit_count = count_set_bits(bitarray, c_bitarray_size);
    ASSERT_EQ(bit_count, 3);

    uint64_t element_value = get_element_at_index(bitarray, c_bitarray_size, c_element_width, c_bitarray_index);
    ASSERT_EQ(element_value, c_element_value);

    size_t found_bitarray_index = find_first_element(bitarray, c_bitarray_size, c_element_width, c_element_value);
    ASSERT_EQ(found_bitarray_index, c_bitarray_index);

    // Stop the search just before the sought element.
    found_bitarray_index = find_first_element(bitarray, c_bitarray_size, c_element_width, c_element_value, c_bitarray_index - 1);
    ASSERT_EQ(found_bitarray_index, c_max_bit_index);
}

TEST(bitarray, conditional_set_element)
{
    constexpr size_t c_bitarray_size = 2;
    std::atomic<uint64_t> bitarray[c_bitarray_size]{};

    constexpr uint64_t c_initial_element_value = 0b0000;
    constexpr uint64_t c_expected_element_value = 0b1101;
    constexpr uint64_t c_desired_element_value = 0b1011;
    constexpr size_t c_element_width = 4;
    constexpr size_t c_bitarray_index = 17;

    uint64_t element_value = get_element_at_index(bitarray, c_bitarray_size, c_element_width, c_bitarray_index);
    ASSERT_EQ(element_value, c_initial_element_value);

    // Verify that the conditional set fails if the expected value is absent.
    bool has_set_value = conditional_set_element_at_index(bitarray, c_bitarray_size, c_element_width, c_bitarray_index, c_expected_element_value, c_desired_element_value);
    ASSERT_FALSE(has_set_value);

    // Verify that the initial value is still present.
    element_value = get_element_at_index(bitarray, c_bitarray_size, c_element_width, c_bitarray_index);
    ASSERT_EQ(element_value, c_initial_element_value);

    // Set expected value and verify it is present.
    set_element_at_index(bitarray, c_bitarray_size, c_element_width, c_bitarray_index, c_expected_element_value);
    element_value = get_element_at_index(bitarray, c_bitarray_size, c_element_width, c_bitarray_index);
    ASSERT_EQ(element_value, c_expected_element_value);

    // We need the same bit ordering as the binary literal representation.
    bool msb_first = true;
    print_bitmap(bitarray, c_bitarray_size, msb_first);

    ASSERT_EQ(bitarray[0], 0);
    ASSERT_EQ(bitarray[1], c_expected_element_value << c_element_width);

    // Verify that the conditional set succeeds if the expected value is present.
    has_set_value = conditional_set_element_at_index(bitarray, c_bitarray_size, c_element_width, c_bitarray_index, c_expected_element_value, c_desired_element_value);
    ASSERT_TRUE(has_set_value);

    // Verify that the desired value is now present.
    element_value = get_element_at_index(bitarray, c_bitarray_size, c_element_width, c_bitarray_index);
    ASSERT_EQ(element_value, c_desired_element_value);

    print_bitmap(bitarray, c_bitarray_size, msb_first);
    ASSERT_EQ(bitarray[0], 0);
    ASSERT_EQ(bitarray[1], c_desired_element_value << c_element_width);

    size_t found_bitarray_index = find_first_element(bitarray, c_bitarray_size, c_element_width, c_desired_element_value);
    ASSERT_EQ(found_bitarray_index, c_bitarray_index);
}
