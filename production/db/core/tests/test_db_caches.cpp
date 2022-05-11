/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "db_caches.hpp"

using namespace gaia::common;
using namespace gaia::db::caches;

TEST(db__core__db_caches, table_relationship_fields_cache_t)
{
    ASSERT_FALSE(table_relationship_fields_cache_t::get()->is_initialized());

    gaia_id_t table_id = 7;
    table_relationship_fields_cache_t::get()->put(table_id);

    ASSERT_TRUE(table_relationship_fields_cache_t::get()->is_initialized());
    ASSERT_EQ(1, table_relationship_fields_cache_t::get()->size());

    field_position_t field1 = 1;
    table_relationship_fields_cache_t::get()->put_parent_relationship_field(table_id, field1);

    field_position_t field3 = 3;
    table_relationship_fields_cache_t::get()->put_child_relationship_field(table_id, field3);

    ASSERT_EQ(1, table_relationship_fields_cache_t::get()->size());

    std::pair<field_position_set_t, field_position_set_t>& fields_pair
        = table_relationship_fields_cache_t::get()->get(table_id);

    ASSERT_TRUE(fields_pair.first.find(field1) != fields_pair.first.end());
    ASSERT_TRUE(fields_pair.second.find(field3) != fields_pair.second.end());

    field_position_t field2 = 2;
    ASSERT_TRUE(fields_pair.first.find(field2) == fields_pair.first.end());
    ASSERT_TRUE(fields_pair.second.find(field2) == fields_pair.second.end());
}
