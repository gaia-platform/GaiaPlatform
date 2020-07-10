/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "field_list.hpp"

using namespace gaia::db::types;

TEST(field_list, set_and_test) {
    field_list_t test_fieldlist = field_list_t(88);
    ASSERT_NO_THROW(test_fieldlist.set(1));
    ASSERT_NO_THROW(test_fieldlist.set(2));
    ASSERT_NO_THROW(test_fieldlist.set(3));
    ASSERT_EQ(test_fieldlist.size(), 3);

    ASSERT_TRUE(test_fieldlist.contains(1));
    ASSERT_TRUE(test_fieldlist.contains(2));
    ASSERT_TRUE(test_fieldlist.contains(3));
    ASSERT_FALSE(test_fieldlist.contains(4));
}

TEST(field_list, clear_test) {
    field_list_t test_list = field_list_t(88);
    test_list.set(1);
    test_list.set(2);
    test_list.set(3);
    test_list.set(4);
    test_list.set(5);
    test_list.set(6);
    test_list.set(7);

    ASSERT_TRUE(test_list.contains(1));
    ASSERT_TRUE(test_list.contains(2));
    ASSERT_TRUE(test_list.contains(3));
    ASSERT_TRUE(test_list.contains(4));
    ASSERT_TRUE(test_list.contains(5));
    ASSERT_TRUE(test_list.contains(6));
    ASSERT_TRUE(test_list.contains(7));

    ASSERT_NO_THROW(test_list.clear(8));

    ASSERT_NO_THROW(test_list.clear(7));
    ASSERT_FALSE(test_list.contains(7));

    ASSERT_NO_THROW(test_list.clear(1));
    ASSERT_FALSE(test_list.contains(1));

    ASSERT_NO_THROW(test_list.clear(3));
    ASSERT_FALSE(test_list.contains(3));

    ASSERT_NO_THROW(test_list.clear(6));
    ASSERT_NO_THROW(test_list.clear(6));
    ASSERT_FALSE(test_list.contains(6));

    ASSERT_TRUE(test_list.contains(2));
    ASSERT_TRUE(test_list.contains(4));
    ASSERT_TRUE(test_list.contains(5));

    ASSERT_EQ(test_list.size(), 3);
}

TEST(field_list, multiple_set) {
    field_list_t test_list = field_list_t(88);
    test_list.set(1);
    test_list.set(2);
    test_list.set(3);

    ASSERT_NO_THROW(test_list.set(1));
    ASSERT_NO_THROW(test_list.set(2));
    ASSERT_NO_THROW(test_list.set(3));

    ASSERT_TRUE(test_list.contains(1));
    ASSERT_TRUE(test_list.contains(2));
    ASSERT_TRUE(test_list.contains(3));

    ASSERT_EQ(test_list.size(), 3);
}

TEST(field_list, emptiness) {
    field_list_t test_list = field_list_t(88);

    ASSERT_TRUE(test_list.empty());
    ASSERT_EQ(test_list.size(), 0);

    test_list.set(1);
    ASSERT_FALSE(test_list.empty());
    ASSERT_EQ(test_list.size(), 1);

    test_list.clear(1);
    ASSERT_TRUE(test_list.empty());
    ASSERT_EQ(test_list.size(), 0);
}
