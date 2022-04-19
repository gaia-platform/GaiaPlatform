/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "db_caches.hpp"

using namespace gaia::common;
using namespace gaia::db::caches;

TEST(db_caches, table_relationship_cache_t)
{
    ASSERT_FALSE(table_relationship_cache_t::get()->is_initialized());

    gaia_id_t table_id = 7;
    table_relationship_cache_t::get()->put(table_id);

    ASSERT_TRUE(table_relationship_cache_t::get()->is_initialized());
    ASSERT_EQ(1, table_relationship_cache_t::get()->size());

    field_position_t field1 = 1;
    table_relationship_cache_t::get()->put(table_id, field1);

    field_position_t field3 = 3;
    table_relationship_cache_t::get()->put(table_id, field3);

    ASSERT_EQ(1, table_relationship_cache_t::get()->size());

    field_position_set_t& fields = table_relationship_cache_t::get()->get(table_id);

    ASSERT_TRUE(fields.find(field1) != fields.end());
    ASSERT_TRUE(fields.find(field3) != fields.end());

    field_position_t field2 = 2;
    ASSERT_TRUE(fields.find(field2) == fields.end());
}
