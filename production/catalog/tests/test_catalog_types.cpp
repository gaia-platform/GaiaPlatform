/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_parser.hpp"

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;
using namespace std;

constexpr char sample_name[] = "my_name";

class gaia_catalog_types_test : public db_catalog_test_base_t
{
protected:
    gaia_catalog_types_test()
        : db_catalog_test_base_t(){};
};

// Insert one row of each catalog table.
TEST_F(gaia_catalog_types_test, create_each_type)
{
    begin_transaction();
    auto database_id = gaia_database_t::insert_row("my_first_database");
    EXPECT_NE(database_id, c_invalid_gaia_id);
    {
        auto table_writer = gaia_table_writer();
        table_writer.name = "my_first_table";
        auto table_id = table_writer.insert_row();
        EXPECT_NE(table_id, c_invalid_gaia_id);
    }
    {
        auto field_writer = gaia_field_writer();
        field_writer.name = "my_first_field";
        auto field_id = field_writer.insert_row();
        EXPECT_NE(field_id, c_invalid_gaia_id);
    }
    {
        auto index_writer = gaia_index_writer();
        index_writer.name = "my_first_index";
        auto index_id = index_writer.insert_row();
        EXPECT_NE(index_id, c_invalid_gaia_id);
    }
    {
        auto relationship_writer = gaia_relationship_writer();
        relationship_writer.name = "my_first_relationship";
        auto relationship_id = relationship_writer.insert_row();
        EXPECT_NE(relationship_id, c_invalid_gaia_id);
    }

    commit_transaction();
}

TEST_F(gaia_catalog_types_test, gaia_database_gaia_table)
{
    begin_transaction();
    // gaia_database
    auto parent_id = gaia_database_t::insert_row(sample_name);
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = sample_name;
    auto child_id = table_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_database_t::get(parent_id);
    parent.gaia_tables().insert(child_id);
    for (auto& it : parent.gaia_tables())
    {
        EXPECT_STREQ(it.name(), sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, gaia_table_gaia_field)
{
    begin_transaction();
    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = sample_name;
    auto parent_id = table_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_field
    auto field_writer = gaia_field_writer();
    field_writer.name = sample_name;
    auto child_id = field_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_table_t::get(parent_id);
    parent.gaia_fields().insert(child_id);
    for (auto& it : parent.gaia_fields())
    {
        EXPECT_STREQ(it.name(), sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, gaia_table_gaia_relationship_parent)
{
    begin_transaction();
    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = sample_name;
    auto parent_id = table_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_relationship
    auto relationship_writer = gaia_relationship_writer();
    relationship_writer.name = sample_name;
    auto child_id = relationship_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_table_t::get(parent_id);
    parent.outgoing_relationships().insert(child_id);
    for (auto& it : parent.outgoing_relationships())
    {
        EXPECT_STREQ(it.name(), sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, gaia_table_gaia_relationship_child)
{
    begin_transaction();
    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = sample_name;
    auto parent_id = table_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_relationship
    auto relationship_writer = gaia_relationship_writer();
    relationship_writer.name = sample_name;
    auto child_id = relationship_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_table_t::get(parent_id);
    parent.incoming_relationships().insert(child_id);
    for (auto& it : parent.incoming_relationships())
    {
        EXPECT_STREQ(it.name(), sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, DISABLED_gaia_ruleset_gaia_rule)
{
    begin_transaction();
    // gaia_ruleset
    auto ruleset_writer = gaia_ruleset_writer();
    ruleset_writer.name = sample_name;
    auto parent_id = ruleset_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_rule
    auto child_id = gaia_rule_t::insert_row(sample_name);
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_ruleset_t::get(parent_id);
    parent.gaia_rules().insert(child_id);
    for (auto& it : parent.gaia_rules())
    {
        EXPECT_STREQ(it.name(), sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, gaia_table_gaia_index)
{
    begin_transaction();
    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = sample_name;
    auto parent_id = table_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_index
    auto index_writer = gaia_index_writer();
    index_writer.name = sample_name;
    auto child_id = index_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_table_t::get(parent_id);
    parent.gaia_indexes().insert(child_id);
    for (auto& it : parent.gaia_indexes())
    {
        EXPECT_STREQ(it.name(), sample_name);
    }
    commit_transaction();
}
