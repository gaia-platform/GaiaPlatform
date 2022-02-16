/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/common/hash.hpp"

using namespace std;
using namespace gaia::common::hash;

constexpr size_t c_test_case_num = 5;
const char* keys[c_test_case_num] = {
    "",
    "abc",
    "zzzzzzzzzz",
    "foo_bar123",
    "The quick brown fox jumps over the lazy dog"};

TEST(hash_test, murmur3_32)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    uint32_t expected_hash_values[c_test_case_num] = {0, 1193954329, 2673600387, 2223942957, 2832703669};

    for (size_t i = 0; i < c_test_case_num; i++)
    {
        const char* key = keys[i];
        size_t len = static_cast<size_t>(strlen(keys[i]));
        uint32_t value = murmur3_32(key, len);
        ASSERT_EQ(value, expected_hash_values[i]);
    }
}

TEST(hash_test, murmur3_128)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    uint8_t expected_hash_values[c_test_case_num][16] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x66, 0xb3, 0xcc, 0x85, 0x4e, 0x58, 0xc4, 0xee, 0x19, 0x0d, 0xb3, 0xc8, 0x75, 0x84, 0x26, 0xe6},
        {0x9e, 0x83, 0x53, 0x76, 0x2c, 0x85, 0x1e, 0xc8, 0x76, 0xef, 0x7a, 0x5f, 0x0e, 0x8a, 0x5c, 0x0b},
        {0xf7, 0xf9, 0xc7, 0xaa, 0x31, 0xa6, 0xf2, 0xb4, 0x72, 0xaa, 0xf4, 0x96, 0xf3, 0xe3, 0x26, 0x1c},
        {0x45, 0x4c, 0xf1, 0x59, 0xe6, 0xbf, 0xca, 0x2a, 0x6d, 0x38, 0x6c, 0xe1, 0x4f, 0x90, 0x96, 0xce}};

    for (size_t i = 0; i < c_test_case_num; i++)
    {
        uint8_t hash_value[c_bytes_per_long_hash];
        murmur3_128(keys[i], strlen(keys[i]), hash_value);
        EXPECT_EQ(memcmp(expected_hash_values[i], hash_value, c_bytes_per_long_hash), 0);
    }

    for (size_t i = 0; i < c_test_case_num; i++)
    {
        multi_segment_hash hashes;
        uint8_t hash_value[c_bytes_per_long_hash];
        hashes.hash_add(keys[i]);
        hashes.hash_calc(hash_value);
        EXPECT_EQ(memcmp(expected_hash_values[i], hash_value, c_bytes_per_long_hash), 0);
    }
}

TEST(hash_test, multi_segment_hash)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    uint8_t expected_hash_values[16]
        = {0x61, 0xd8, 0x35, 0x12, 0x6d, 0x38, 0x2e, 0x3a, 0x89, 0xd6, 0x70, 0xa9, 0xae, 0xb4, 0x4c, 0x7b};

    multi_segment_hash hashes;
    uint8_t hash_value[c_bytes_per_long_hash];

    for (size_t i = 0; i < c_test_case_num; i++)
    {
        hashes.hash_add(keys[i]);
    }

    hashes.hash_calc(hash_value);
    EXPECT_EQ(std::memcmp(expected_hash_values, hash_value, c_bytes_per_long_hash), 0);
}
