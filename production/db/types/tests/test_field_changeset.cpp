/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "field_changeset.hpp"

using namespace gaia::db::types;

TEST(field_changeset, set_and_test) {
    field_changeset_t test_changeset = field_changeset_t(88);
    ASSERT_NO_THROW(test_changeset.set(1));
    ASSERT_NO_THROW(test_changeset.set(2));
    ASSERT_NO_THROW(test_changeset.set(3));
    ASSERT_EQ(test_changeset.size(), 3);

    ASSERT_TRUE(test_changeset.test(1));
    ASSERT_TRUE(test_changeset.test(2));
    ASSERT_TRUE(test_changeset.test(3));
    ASSERT_FALSE(test_changeset.test(4));
}

TEST(field_changeset, clear_test) {
    field_changeset_t test_changeset = field_changeset_t(88);
    test_changeset.set(1);
    test_changeset.set(2);
    test_changeset.set(3);
    test_changeset.set(4);
    test_changeset.set(5);
    test_changeset.set(6);
    test_changeset.set(7);

    ASSERT_TRUE(test_changeset.test(1));
    ASSERT_TRUE(test_changeset.test(2));
    ASSERT_TRUE(test_changeset.test(3));
    ASSERT_TRUE(test_changeset.test(4));
    ASSERT_TRUE(test_changeset.test(5));
    ASSERT_TRUE(test_changeset.test(6));
    ASSERT_TRUE(test_changeset.test(7));

    ASSERT_NO_THROW(test_changeset.clear(8));

    ASSERT_NO_THROW(test_changeset.clear(7));
    ASSERT_FALSE(test_changeset.test(7));

    ASSERT_NO_THROW(test_changeset.clear(1));
    ASSERT_FALSE(test_changeset.test(1));

    ASSERT_NO_THROW(test_changeset.clear(3));
    ASSERT_FALSE(test_changeset.test(3));

    ASSERT_NO_THROW(test_changeset.clear(6));
    ASSERT_NO_THROW(test_changeset.clear(6));
    ASSERT_FALSE(test_changeset.test(6));

    ASSERT_TRUE(test_changeset.test(2));
    ASSERT_TRUE(test_changeset.test(4));
    ASSERT_TRUE(test_changeset.test(5));

    ASSERT_EQ(test_changeset.size(), 3);
}

TEST(field_changeset, multiple_set) {
    field_changeset_t test_changeset = field_changeset_t(88);
    test_changeset.set(1);
    test_changeset.set(2);
    test_changeset.set(3);

    ASSERT_NO_THROW(test_changeset.set(1));
    ASSERT_NO_THROW(test_changeset.set(2));
    ASSERT_NO_THROW(test_changeset.set(3));

    ASSERT_TRUE(test_changeset.test(1));
    ASSERT_TRUE(test_changeset.test(2));
    ASSERT_TRUE(test_changeset.test(3));

    ASSERT_EQ(test_changeset.size(), 3);
}

TEST(field_changeset, emptiness) {
    field_changeset_t test_changeset = field_changeset_t(88);

    ASSERT_TRUE(test_changeset.empty());
    ASSERT_EQ(test_changeset.size(), 0);

    test_changeset.set(1);
    ASSERT_FALSE(test_changeset.empty());
    ASSERT_EQ(test_changeset.size(), 1);

    test_changeset.clear(1);
    ASSERT_TRUE(test_changeset.empty());
    ASSERT_EQ(test_changeset.size(), 0);
}
