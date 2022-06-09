////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <atomic>

#include <gtest/gtest.h>

#include <gaia/common.hpp>

#include "gaia_internal/common/bitmap.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::common::bitmap;

TEST(common__bitmap__test, set_bit_value_and_is_bit_set)
{
    constexpr size_t c_bitmap_size_in_words = 3;
    std::atomic<uint64_t> bitmap[c_bitmap_size_in_words];

    // Start with an empty bitmap.
    // Set each bit of the bitmap and verify that it was set and that only one bit was set.
    bitmap[0] = bitmap[1] = bitmap[2] = 0;
    for (size_t i = 0; i < c_bitmap_size_in_words * c_uint64_bit_count; ++i)
    {
        set_bit_value(bitmap, c_bitmap_size_in_words, i, true);
        ASSERT_EQ(true, is_bit_set(bitmap, c_bitmap_size_in_words, i));
        ASSERT_EQ(1, count_set_bits(bitmap, c_bitmap_size_in_words));
        bitmap[i / c_uint64_bit_count] = 0;
    }

    // Start with a full bitmap.
    // Unset each bit of the bitmap and verify that it was unset and that only one bit was unset.
    bitmap[0] = bitmap[1] = bitmap[2] = -1;
    for (size_t i = 0; i < c_bitmap_size_in_words * c_uint64_bit_count; ++i)
    {
        set_bit_value(bitmap, c_bitmap_size_in_words, i, false);
        ASSERT_EQ(false, is_bit_set(bitmap, c_bitmap_size_in_words, i));
        ASSERT_EQ(c_bitmap_size_in_words * c_uint64_bit_count - 1, count_set_bits(bitmap, c_bitmap_size_in_words));
        bitmap[i / c_uint64_bit_count] = -1;
    }
}

TEST(common__bitmap__test, set_already_set_bit_value)
{
    constexpr size_t c_bitmap_size_in_words = 3;
    std::atomic<uint64_t> bitmap[c_bitmap_size_in_words];

    // Start with an empty bitmap.
    // Set each bit of the bitmap and verify that it was set and that only one bit was set.
    bitmap[0] = bitmap[1] = bitmap[2] = 0;
    for (size_t i = 0; i < c_bitmap_size_in_words * c_uint64_bit_count; ++i)
    {
        bool success = try_set_bit_value(bitmap, c_bitmap_size_in_words, i, true);
        ASSERT_TRUE(success);
        ASSERT_EQ(true, is_bit_set(bitmap, c_bitmap_size_in_words, i));
        ASSERT_EQ(1, count_set_bits(bitmap, c_bitmap_size_in_words));
        bool fail_if_already_set = true;
        // try_set_bit_value() should fail if the bit is already set and fail_if_already_set=true.
        success = try_set_bit_value(bitmap, c_bitmap_size_in_words, i, true, fail_if_already_set);
        ASSERT_FALSE(success);
        bitmap[i / c_uint64_bit_count] = 0;
    }

    // Start with a full bitmap.
    // Unset each bit of the bitmap and verify that it was unset and that only one bit was unset.
    bitmap[0] = bitmap[1] = bitmap[2] = -1;
    for (size_t i = 0; i < c_bitmap_size_in_words * c_uint64_bit_count; ++i)
    {
        bool success = try_set_bit_value(bitmap, c_bitmap_size_in_words, i, false);
        ASSERT_TRUE(success);
        ASSERT_EQ(false, is_bit_set(bitmap, c_bitmap_size_in_words, i));
        ASSERT_EQ(c_bitmap_size_in_words * c_uint64_bit_count - 1, count_set_bits(bitmap, c_bitmap_size_in_words));
        bool fail_if_already_set = true;
        // try_set_bit_value() should fail if the bit is already unset and fail_if_already_set=true.
        success = try_set_bit_value(bitmap, c_bitmap_size_in_words, i, false, fail_if_already_set);
        ASSERT_FALSE(success);
        bitmap[i / c_uint64_bit_count] = -1;
    }
}

TEST(common__bitmap__test, find_first_unset_bit)
{
    constexpr size_t c_bitmap_size_in_words = 3;
    std::atomic<uint64_t> bitmap[c_bitmap_size_in_words] = {0};

    // Start with an empty bitmap.
    // Keep setting bits in the first two words of the bitmap
    // and verify that find_first_unset_bit finds the next unset bit.
    for (size_t i = 0; i < (c_bitmap_size_in_words - 1) * c_uint64_bit_count; ++i)
    {
        size_t bit_index_in_word = i % c_uint64_bit_count;
        if (bit_index_in_word != 0)
        {
            bitmap[i / c_uint64_bit_count] = (1UL << bit_index_in_word) - 1;
        }

        ASSERT_EQ(i, find_first_unset_bit(bitmap, c_bitmap_size_in_words));

        // After we're done with a word, leave all bits set,
        // so the search will go into the next word.
        if (bit_index_in_word == c_uint64_bit_count - 1)
        {
            bitmap[i / c_uint64_bit_count] = -1;
        }
    }

    // Check that we have set the bits of the first 2 words of the bitmap.
    ASSERT_EQ((c_bitmap_size_in_words - 1) * c_uint64_bit_count, count_set_bits(bitmap, c_bitmap_size_in_words));
}

TEST(common__bitmap__test, find_last_set_bit)
{
    constexpr size_t c_bitmap_size_in_words = 3;
    std::atomic<uint64_t> bitmap[c_bitmap_size_in_words] = {0};
    constexpr size_t bitmap_size_in_bits = c_bitmap_size_in_words * c_uint64_bit_count;

    // Start with an empty bitmap. Set each bit in the bitmap from left to
    // right, and verify that find_last_set_bit() finds the last set bit.
    for (size_t i = 0; i < bitmap_size_in_bits; ++i)
    {
        set_bit_value(bitmap, c_bitmap_size_in_words, i, true);
        ASSERT_EQ(i, find_last_set_bit(bitmap, c_bitmap_size_in_words));
    }

    // Check that we have set all bits.
    ASSERT_EQ(bitmap_size_in_bits, count_set_bits(bitmap, c_bitmap_size_in_words));
}

TEST(common__bitmap__test, limit)
{
    std::atomic<uint64_t> bitmap = 0;
    size_t end_limit_bit_index = 8;

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
            bitmap = (1UL << i) - 1;
        }

        if (i < end_limit_bit_index)
        {
            ASSERT_EQ(i, find_first_unset_bit(&bitmap, 1, end_limit_bit_index));
        }
        else
        {
            ASSERT_EQ(c_max_bit_index, find_first_unset_bit(&bitmap, 1, end_limit_bit_index));
        }

        if (i < end_limit_bit_index)
        {
            ASSERT_EQ(i, count_set_bits(&bitmap, 1, end_limit_bit_index));
        }
        else
        {
            ASSERT_EQ(end_limit_bit_index, count_set_bits(&bitmap, 1, end_limit_bit_index));
        }
    }

    // Check that we have set the bits of the bitmap.
    ASSERT_EQ(end_limit_bit_index, count_set_bits(&bitmap, 1, end_limit_bit_index));
}

TEST(common__bitmap__test, count_set_bits)
{
    constexpr size_t c_bitmap_size_in_words = 3;
    std::atomic<uint64_t> bitmap[c_bitmap_size_in_words];

    // Start with an empty bitmap.
    // Keep setting bits in the first two words of the bitmap
    // and verify that count_set_bits counts them properly.
    bitmap[0] = bitmap[1] = bitmap[2] = 0;
    for (size_t i = 0; i < (c_bitmap_size_in_words - 1) * c_uint64_bit_count; ++i)
    {
        size_t bit_index_in_word = i % c_uint64_bit_count;
        if (bit_index_in_word != 0)
        {
            bitmap[i / c_uint64_bit_count] = (1UL << bit_index_in_word) - 1;
        }

        ASSERT_EQ(i, count_set_bits(bitmap, c_bitmap_size_in_words));

        if (bit_index_in_word == c_uint64_bit_count - 1)
        {
            bitmap[i / c_uint64_bit_count] = -1;
        }
    }

    // Check that we have set the bits of the first 2 words of the bitmap.
    ASSERT_EQ((c_bitmap_size_in_words - 1) * c_uint64_bit_count, count_set_bits(bitmap, c_bitmap_size_in_words));
}

TEST(common__bitmap__test, bit_setting)
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

TEST(common__bitmap__test, bit_range_setting)
{
    constexpr size_t c_bitmap_size_in_words = 5;
    std::atomic<uint64_t> bitmap[c_bitmap_size_in_words]{0};

    safe_set_bit_range_value(bitmap, c_bitmap_size_in_words, 3, 3, true);
    print_bitmap(bitmap, 1);
    ASSERT_EQ(bitmap[0], 56);

    size_t bit_count = count_set_bits(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bit_count, 3);

    size_t bit_index = find_first_unset_bit(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bit_index, 0);

    safe_set_bit_range_value(bitmap, c_bitmap_size_in_words, 2, 5, false);
    print_bitmap(bitmap, 1);
    ASSERT_EQ(bitmap[0], 0);

    bit_count = count_set_bits(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bit_count, 0);

    safe_set_bit_range_value(bitmap, c_bitmap_size_in_words, 10, 65, true);
    print_bitmap(bitmap, c_bitmap_size_in_words);

    bit_count = count_set_bits(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bit_count, 65);

    bit_index = find_first_unset_bit(&bitmap[1], 1);
    ASSERT_EQ(bit_index, 11);

    safe_set_bit_range_value(bitmap, c_bitmap_size_in_words, 11, 63, false);
    print_bitmap(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bitmap[0], 1024);
    ASSERT_EQ(bitmap[1], 1024);

    bit_count = count_set_bits(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bit_count, 2);

    safe_set_bit_range_value(bitmap, c_bitmap_size_in_words, 138, 129, true);
    print_bitmap(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bitmap[3], -1);

    bit_count = count_set_bits(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bit_count, 131);

    bit_index = find_first_unset_bit(&bitmap[3], 2);
    ASSERT_EQ(bit_index, 75);

    safe_set_bit_range_value(bitmap, c_bitmap_size_in_words, 139, 127, false);
    print_bitmap(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bitmap[2], 1024);
    ASSERT_EQ(bitmap[3], 0);
    ASSERT_EQ(bitmap[4], 1024);

    bit_count = count_set_bits(bitmap, c_bitmap_size_in_words);
    ASSERT_EQ(bit_count, 4);
}
