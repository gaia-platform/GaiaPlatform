/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include "data_holder.hpp"

using namespace std;
using namespace gaia::db::payload_types;

constexpr uint64_t c_unsigned_integer_value = 7;
constexpr int64_t c_integer_value = 7;
constexpr int64_t c_negated_integer_value = -c_integer_value;

constexpr double c_float_value = -12.345;
constexpr double c_another_float_value = 67.890;

TEST(db__payload_types__data_holder__test, is_signed_integer)
{
    ASSERT_TRUE(is_signed_integer(reflection::Byte));
    ASSERT_FALSE(is_signed_integer(reflection::UByte));
    ASSERT_TRUE(is_signed_integer(reflection::Short));
    ASSERT_FALSE(is_signed_integer(reflection::UShort));
    ASSERT_TRUE(is_signed_integer(reflection::Int));
    ASSERT_FALSE(is_signed_integer(reflection::UInt));
    ASSERT_TRUE(is_signed_integer(reflection::Long));
    ASSERT_FALSE(is_signed_integer(reflection::ULong));

    ASSERT_FALSE(is_signed_integer(reflection::String));
}

TEST(db__payload_types__data_holder__test, string)
{
    data_holder_t value;
    data_holder_t other_value;

    value.type = other_value.type = reflection::String;
    value.hold.string_value = "Alice";
    value.is_null = false;
    other_value.hold.string_value = "Alyssa";
    other_value.is_null = false;

    ASSERT_TRUE(value.compare(other_value) < 0);
    ASSERT_TRUE(other_value.compare(value) > 0);
    ASSERT_EQ(0, value.compare(value));
}

TEST(db__payload_types__data_holder__test, vector)
{
    data_holder_t value;
    data_holder_t other_value;

    value.type = other_value.type = reflection::Vector;
    value.hold.vector_value = "Aloha";
    value.is_null = false;
    other_value.hold.vector_value = "Hello";
    other_value.is_null = false;

    ASSERT_TRUE(value.compare(other_value) < 0);
    ASSERT_TRUE(other_value.compare(value) > 0);
    ASSERT_EQ(0, value.compare(value));

    value.hold.vector_value = "BATMAN";
    other_value.hold.vector_value = "NANANANANANA";
    ASSERT_TRUE(value.compare(other_value) < 0);
    ASSERT_TRUE(other_value.compare(value) > 0);
    ASSERT_EQ(0, value.compare(value));

    value.hold.vector_value = "";
    other_value.hold.vector_value = "A";
    ASSERT_TRUE(value.compare(other_value) < 0);
    ASSERT_TRUE(other_value.compare(value) > 0);
    ASSERT_EQ(0, value.compare(value));
}

TEST(db__payload_types__data_holder__test, signed_integer)
{
    data_holder_t value;
    data_holder_t other_value;

    // Test signed comparison.
    value.type = other_value.type = reflection::Int;
    value.hold.integer_value = c_negated_integer_value;
    value.is_null = false;
    other_value.hold.integer_value = c_integer_value;
    other_value.is_null = false;

    ASSERT_EQ(-1, value.compare(other_value));
    ASSERT_EQ(1, other_value.compare(value));
    ASSERT_EQ(0, value.compare(value));
}

TEST(db__payload_types__data_holder__test, unsigned_integer)
{
    data_holder_t value;
    data_holder_t other_value;

    // Test unsigned comparison.
    value.type = other_value.type = reflection::UInt;
    value.hold.integer_value = c_negated_integer_value;
    value.is_null = false;
    other_value.hold.integer_value = c_integer_value;
    other_value.is_null = false;

    ASSERT_EQ(1, value.compare(other_value));
    ASSERT_EQ(-1, other_value.compare(value));
    ASSERT_EQ(0, value.compare(value));
}

TEST(db__payload_types__data_holder__test, float)
{
    data_holder_t value;
    data_holder_t other_value;

    value.type = other_value.type = reflection::Float;
    value.is_null = false;
    value.hold.float_value = c_float_value;
    other_value.hold.float_value = c_another_float_value;
    other_value.is_null = false;

    ASSERT_EQ(-1, value.compare(other_value));
    ASSERT_EQ(1, other_value.compare(value));
    ASSERT_EQ(0, value.compare(value));
}

TEST(db__payload_types__data_holder__test, box_unbox)
{
    data_holder_t float_value = c_float_value;
    data_holder_t signed_float_value = c_another_float_value;
    float f;

    // Check for equality accounting for floating point error.
    f = float_value;
    ASSERT_EQ(f, static_cast<float>(c_float_value));

    f = signed_float_value;
    ASSERT_EQ(f, static_cast<float>(c_another_float_value));

    double d;
    d = float_value;
    ASSERT_EQ(d, c_float_value);

    d = signed_float_value;
    ASSERT_EQ(d, c_another_float_value);

    data_holder_t integer_value = c_integer_value;
    data_holder_t signed_integer_value = c_negated_integer_value;
    data_holder_t unsigned_integer_value = c_unsigned_integer_value;

    int64_t i64 = integer_value;
    ASSERT_EQ(i64, c_integer_value);
    // Type mismatch.
    ASSERT_THROW(i64 = float_value, unboxing_error);

    i64 = signed_integer_value;
    ASSERT_EQ(i64, c_negated_integer_value);

    int32_t i32 = integer_value;
    ASSERT_EQ(i32, c_integer_value);

    i32 = signed_integer_value;
    ASSERT_EQ(i32, c_negated_integer_value);

    uint64_t u64;
    // Should throw due to sign mismatch.
    ASSERT_THROW(u64 = integer_value, unboxing_error);
    u64 = unsigned_integer_value;
    ASSERT_EQ(u64, c_unsigned_integer_value);

    uint32_t u32;

    // Should throw due to sign mismatch.
    ASSERT_THROW(u32 = integer_value, unboxing_error);
    u32 = unsigned_integer_value;
    ASSERT_EQ(u32, c_unsigned_integer_value);
}

TEST(db__payload_types__data_holder__test, box_unbox_string)
{
    data_holder_t value = "Hello";
    const char* str = value;

    ASSERT_TRUE(strcmp(str, "Hello") == 0);
}

TEST(db__payload_types__data_holder__test, null_empty_string_hash_compare)
{
    data_holder_t value = "";
    data_holder_t nullstring;
    nullstring.type = reflection::String;
    nullstring.is_null = true;
    nullstring.hold.string_value = nullptr;

    ASSERT_TRUE(value.hash() != nullstring.hash());
}
