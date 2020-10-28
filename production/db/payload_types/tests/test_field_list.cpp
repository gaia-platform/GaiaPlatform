/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <vector>

#include "gtest/gtest.h"

#include "db_test_base.hpp"
#include "field_list.hpp"

using namespace std;
using namespace gaia::db::payload_types;

constexpr gaia::common::gaia_id_t c_type_id = 888;

class field_list : public db_test_base_t
{
};

TEST_F(field_list, add_and_test)
{
    field_list_t test_field_list = field_list_t(c_type_id);
    ASSERT_NO_THROW(test_field_list.add(1));
    ASSERT_NO_THROW(test_field_list.add(2));
    ASSERT_NO_THROW(test_field_list.add(3));
    ASSERT_EQ(test_field_list.size(), 3);

    ASSERT_TRUE(test_field_list.contains(1));
    ASSERT_TRUE(test_field_list.contains(2));
    ASSERT_TRUE(test_field_list.contains(3));
    ASSERT_FALSE(test_field_list.contains(4));
}

TEST_F(field_list, multi_add)
{
    field_list_t test_list = field_list_t(c_type_id);
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

TEST_F(field_list, emptiness)
{
    field_list_t test_list = field_list_t(c_type_id);

    ASSERT_TRUE(test_list.is_empty());
    ASSERT_EQ(test_list.size(), 0);

    test_list.add(1);
    ASSERT_FALSE(test_list.is_empty());
    ASSERT_EQ(test_list.size(), 1);
}
