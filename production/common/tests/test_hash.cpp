/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/common/hash.hpp"

TEST(hash_test, murmur3_32)
{
    constexpr size_t c_test_case_num = 5;
    const char* keys[c_test_case_num] = {
        "",
        "abc",
        "zzzzzzzzzz",
        "foo_bar123",
        "The quick brown fox jumps over the lazy dog"};
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    uint32_t expected_hash_values[c_test_case_num] = {0, 1193954329, 2673600387, 2223942957, 2832703669};

    for (size_t i = 0; i < c_test_case_num; i++)
    {
        const char* key = keys[i];
        int len = static_cast<int>(strlen(keys[i]));
        uint32_t value = gaia::common::hash::murmur3_32(key, len);
        ASSERT_EQ(value, expected_hash_values[i]);
    }
}
