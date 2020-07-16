/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "field_list.hpp"

using namespace gaia::db::types;

TEST(field_list, add_and_test) {
    field_list_t test_fieldlist = field_list_t(888);
    ASSERT_NO_THROW(test_fieldlist.add(1));
    ASSERT_NO_THROW(test_fieldlist.add(2));
    ASSERT_NO_THROW(test_fieldlist.add(3));
    ASSERT_EQ(test_fieldlist.size(), 3);

    ASSERT_TRUE(test_fieldlist.contains(1));
    ASSERT_TRUE(test_fieldlist.contains(2));
    ASSERT_TRUE(test_fieldlist.contains(3));
    ASSERT_FALSE(test_fieldlist.contains(4));
}

TEST(field_list, multi_add) {
    field_list_t test_list = field_list_t(888);
    test_list.add(1);
    test_list.add(2);
    test_list.add(3);

    ASSERT_NO_THROW(test_list.add(1));
    ASSERT_NO_THROW(test_list.add(2));
    ASSERT_NO_THROW(test_list.add(3));

    ASSERT_TRUE(test_list.contains(1));
    ASSERT_TRUE(test_list.contains(2));
    ASSERT_TRUE(test_list.contains(3));

    ASSERT_EQ(test_list.size(), 3);
}

TEST(field_list, emptiness) {
    field_list_t test_list = field_list_t(888);

    ASSERT_TRUE(test_list.is_empty());
    ASSERT_EQ(test_list.size(), 0);

    test_list.add(1);
    ASSERT_FALSE(test_list.is_empty());
    ASSERT_EQ(test_list.size(), 1);
}
