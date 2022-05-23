////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include "db_caches.hpp"

using namespace gaia::common;
using namespace gaia::db::caches;

TEST(db__core__db_caches__test, table_relationship_fields_cache_t)
{
    table_relationship_fields_cache_t table_relationship_fields_cache;

    ASSERT_FALSE(table_relationship_fields_cache.is_initialized());

    // Create a table entry and verify its creation.
    gaia_id_t table_id = 7;
    table_relationship_fields_cache.put(table_id);

    ASSERT_TRUE(table_relationship_fields_cache.is_initialized());
    ASSERT_EQ(1, table_relationship_fields_cache.size());

    // Add two fields to the table entry.
    field_position_t field1 = 1;
    table_relationship_fields_cache.put_parent_relationship_field(table_id, field1);

    field_position_t field3 = 3;
    table_relationship_fields_cache.put_child_relationship_field(table_id, field3);

    // We should still have a single entry.
    ASSERT_EQ(1, table_relationship_fields_cache.size());

    // Check that the field entries are stored separately in the pair.
    const auto& fields_pair = table_relationship_fields_cache.get(table_id);

    ASSERT_TRUE(fields_pair.first.find(field1) != fields_pair.first.end());
    ASSERT_TRUE(fields_pair.second.find(field3) != fields_pair.second.end());

    field_position_t field2 = 2;
    ASSERT_TRUE(fields_pair.first.find(field2) == fields_pair.first.end());
    ASSERT_TRUE(fields_pair.second.find(field2) == fields_pair.second.end());

    // Clear the cache.
    table_relationship_fields_cache.clear();

    ASSERT_FALSE(table_relationship_fields_cache.is_initialized());
    ASSERT_EQ(0, table_relationship_fields_cache.size());
}
