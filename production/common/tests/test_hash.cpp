////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/common/hash.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::common::hash;

constexpr size_t c_test_case_num = 5;
const char* keys[c_test_case_num] = {
    "",
    "abc",
    "zzzzzzzzzz",
    "foo_bar123",
    "The quick brown fox jumps over the lazy dog"};

TEST(common__hash__test, murmur3_32)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    uint32_t expected_hash_values[c_test_case_num] = {0, 1193954329, 2673600387, 2223942957, 2832703669};

    for (size_t i = 0; i < c_test_case_num; i++)
    {
        const char* key = keys[i];
        int len = static_cast<size_t>(strlen(keys[i]));
        uint32_t value = murmur3_32(key, len);
        ASSERT_EQ(value, expected_hash_values[i]);
    }
}

TEST(common__hash__test, murmur3_128)
{
    uint8_t expected_hash_values[c_test_case_num][multi_segment_hash::c_murmur3_128_hash_size_in_bytes] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x66, 0xb3, 0xcc, 0x85, 0x4e, 0x58, 0xc4, 0xee, 0x19, 0x0d, 0xb3, 0xc8, 0x75, 0x84, 0x26, 0xe6},
        {0x9e, 0x83, 0x53, 0x76, 0x2c, 0x85, 0x1e, 0xc8, 0x76, 0xef, 0x7a, 0x5f, 0x0e, 0x8a, 0x5c, 0x0b},
        {0xf7, 0xf9, 0xc7, 0xaa, 0x31, 0xa6, 0xf2, 0xb4, 0x72, 0xaa, 0xf4, 0x96, 0xf3, 0xe3, 0x26, 0x1c},
        {0x45, 0x4c, 0xf1, 0x59, 0xe6, 0xbf, 0xca, 0x2a, 0x6d, 0x38, 0x6c, 0xe1, 0x4f, 0x90, 0x96, 0xce}};

    for (size_t i = 0; i < c_test_case_num; i++)
    {
        multi_segment_hash hashes;
        uint8_t hash_value[multi_segment_hash::c_murmur3_128_hash_size_in_bytes];
        hashes.hash_add(keys[i]);
        hashes.hash_calc(hash_value);
        EXPECT_EQ(memcmp(expected_hash_values[i], hash_value, multi_segment_hash::c_murmur3_128_hash_size_in_bytes), 0);
    }
}

TEST(common__hash__test, multi_segment_hash)
{
    uint8_t expected_hash_values[multi_segment_hash::c_murmur3_128_hash_size_in_bytes]
        = {0x61, 0xd8, 0x35, 0x12, 0x6d, 0x38, 0x2e, 0x3a, 0x89, 0xd6, 0x70, 0xa9, 0xae, 0xb4, 0x4c, 0x7b};
    // This is the ASCII representation of the hex bytes above.
    const char expected_hash_string[] = "61d835126d382e3a89d670a9aeb44c7b";

    multi_segment_hash hashes;
    uint8_t hash_value[multi_segment_hash::c_murmur3_128_hash_size_in_bytes];

    // Mash all of the keys into a single hash.
    for (size_t i = 0; i < c_test_case_num; i++)
    {
        hashes.hash_add(keys[i]);
    }

    // Lock in the hash value and compare to known result.
    hashes.hash_calc(hash_value);
    EXPECT_EQ(std::memcmp(expected_hash_values, hash_value, multi_segment_hash::c_murmur3_128_hash_size_in_bytes), 0);

    // Compare the ASCII representation to the object's string conversion.
    char* hash_string = hashes.to_string();
    EXPECT_STREQ(hash_string, expected_hash_string);

    // Convert the string back back to bytes and compare again.
    uint8_t converted_hash_values[multi_segment_hash::c_murmur3_128_hash_size_in_bytes];
    char one_byte[3];
    uint32_t int_byte;
    for (size_t byte = 0; byte < multi_segment_hash::c_murmur3_128_hash_size_in_bytes; ++byte)
    {
        one_byte[0] = hash_string[byte * 2];
        one_byte[1] = hash_string[byte * 2 + 1];
        one_byte[2] = '\0';
        sscanf(one_byte, "%2x", &int_byte);
        converted_hash_values[byte] = static_cast<uint8_t>(int_byte);
    }
    EXPECT_EQ(std::memcmp(expected_hash_values, converted_hash_values, multi_segment_hash::c_murmur3_128_hash_size_in_bytes), 0);
}

TEST(common__hash__test, bad_keys)
{
    // This should not result in an exception.
    murmur3_32(nullptr, 0);

    multi_segment_hash hashes;
    // A null cstring or uint8_t pointer should result in an excepton.
    const char* null_cstring = nullptr;
    uint8_t* null_byte_ptr = nullptr;
    EXPECT_THROW(hashes.hash_add(null_cstring), assertion_failure);
    EXPECT_THROW(hashes.hash_include(null_byte_ptr), assertion_failure);
}
