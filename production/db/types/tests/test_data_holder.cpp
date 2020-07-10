/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "data_holder.hpp"

using namespace std;
using namespace gaia::db::types;

TEST(types, is_signed_integer)
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

TEST(types, type_holder_string)
{
    data_holder_t value;
    data_holder_t other_value;

    value.type = other_value.type = reflection::String;
    value.hold.string_value = "Alice";
    other_value.hold.string_value = "Alyssa";

    ASSERT_TRUE(value.compare(other_value) < 0);
    ASSERT_TRUE(other_value.compare(value) > 0);
    ASSERT_EQ(0, value.compare(value));
}

TEST(types, type_holder_signed_integer)
{
    data_holder_t value;
    data_holder_t other_value;

    // Test signed comparison.
    value.type = other_value.type = reflection::Int;
    value.hold.integer_value = -7;
    other_value.hold.integer_value = 7;

    ASSERT_EQ(-1, value.compare(other_value));
    ASSERT_EQ(1, other_value.compare(value));
    ASSERT_EQ(0, value.compare(value));
}

TEST(types, type_holder_unsigned_integer)
{
    data_holder_t value;
    data_holder_t other_value;

    // Test unsigned comparison.
    value.type = other_value.type = reflection::UInt;
    value.hold.integer_value = -7;
    other_value.hold.integer_value = 7;

    ASSERT_EQ(1, value.compare(other_value));
    ASSERT_EQ(-1, other_value.compare(value));
    ASSERT_EQ(0, value.compare(value));
}

TEST(types, type_holder_float)
{
    data_holder_t value;
    data_holder_t other_value;

    value.type = other_value.type = reflection::Float;
    value.hold.float_value = -12.345;
    other_value.hold.float_value = 67.890;

    ASSERT_EQ(-1, value.compare(other_value));
    ASSERT_EQ(1, other_value.compare(value));
    ASSERT_EQ(0, value.compare(value));
}
