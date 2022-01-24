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

constexpr char c_sample_name[] = "my_name";
constexpr char c_hash_value[] = "hash value";
constexpr uint32_t c_sample_table_type = 0;
constexpr uint8_t c_sample_field_type = 0;
constexpr bool c_sample_boolean = true;

class gaia_core_tables_test : public db_catalog_test_base_t
{
protected:
    gaia_core_tables_test()
        : db_catalog_test_base_t(){};
};

// Core types include gaia_database_t, gaia_table_t, gaia_field_t, gaia_relationship_t, gaia_index_t.
// Insert each one two ways.
TEST_F(gaia_core_tables_test, create_core_table_types)
{
    begin_transaction();

    // gaia_database_t
    {
        auto database_writer = gaia_database_writer();
        database_writer.name = c_sample_name;
        auto database_id = database_writer.insert_row();
        EXPECT_NE(database_id, c_invalid_gaia_id);
    }
    auto database_id = gaia_database_t::insert_row(c_sample_name, c_hash_value);
    EXPECT_NE(database_id, c_invalid_gaia_id);

    // gaia_table_t
    {
        auto table_writer = gaia_table_writer();
        table_writer.name = c_sample_name;
        auto table_id = table_writer.insert_row();
        EXPECT_NE(table_id, c_invalid_gaia_id);
    }
    auto gaia_table_id = gaia_table_t::insert_row(
        c_sample_name,
        c_sample_table_type,
        c_sample_boolean,
        std::vector<uint8_t>{1, 2, 3},
        std::vector<uint8_t>{4, 5, 6, 7, 8},
        c_hash_value);
    EXPECT_NE(gaia_table_id, c_invalid_gaia_id);

    // gaia_field_t
    {
        auto field_writer = gaia_field_writer();
        field_writer.name = c_sample_name;
        auto field_id = field_writer.insert_row();
        EXPECT_NE(field_id, c_invalid_gaia_id);
    }
    auto gaia_field_id = gaia_field_t::insert_row(
        c_sample_name,
        c_sample_field_type,
        1,
        2,
        c_sample_boolean,
        c_sample_boolean,
        c_sample_boolean,
        c_hash_value,
        c_sample_boolean);
    EXPECT_NE(gaia_field_id, c_invalid_gaia_id);

    // gaia_index_t
    {
        auto index_writer = gaia_index_writer();
        index_writer.name = c_sample_name;
        auto index_id = index_writer.insert_row();
        EXPECT_NE(index_id, c_invalid_gaia_id);
    }
    auto gaia_index_id = gaia_index_t::insert_row(
        c_sample_name,
        c_sample_boolean,
        c_sample_field_type,
        std::vector<uint64_t>{1, 2, 3},
        c_hash_value);
    EXPECT_NE(gaia_index_id, c_invalid_gaia_id);

    // gaia_relationship_t
    {
        auto relationship_writer = gaia_relationship_writer();
        relationship_writer.name = c_sample_name;
        auto relationship_id = relationship_writer.insert_row();
        EXPECT_NE(relationship_id, c_invalid_gaia_id);
    }
    auto gaia_relationship_id = gaia_relationship_t::insert_row(
        c_sample_name,
        "rules",
        "ruleset",
        1,
        c_sample_boolean,
        c_sample_boolean,
        0,
        1,
        2,
        0,
        std::vector<uint16_t>{12, 13, 14, 15},
        std::vector<uint16_t>{101, 202, 303},
        c_hash_value);
    EXPECT_NE(gaia_relationship_id, c_invalid_gaia_id);

    commit_transaction();
}

// System table types include gaia_ruleset_t and gaia_rule_t.
// Insert each one two ways.
TEST_F(gaia_core_tables_test, DISABLED_create_system_table_types)
{
    begin_transaction();

    // gaia_ruleset_t
    {
        auto ruleset_writer = gaia_ruleset_writer();
        ruleset_writer.name = c_sample_name;
        auto ruleset_id = ruleset_writer.insert_row();
        EXPECT_NE(ruleset_id, c_invalid_gaia_id);
    }
    auto ruleset_id = gaia_ruleset_t::insert_row(
        c_sample_name,
        "serial_stream");
    EXPECT_NE(ruleset_id, c_invalid_gaia_id);

    // gaia_rule_t
    {
        auto rule_writer = gaia_rule_writer();
        rule_writer.name = c_sample_name;
        auto rule_id = rule_writer.insert_row();
        EXPECT_NE(rule_id, c_invalid_gaia_id);
    }
    auto rule_id = gaia_rule_t::insert_row(c_sample_name);
    EXPECT_NE(rule_id, c_invalid_gaia_id);

    // gaia_ref_anchor_t
    {
        auto ref_anchor_writer = gaia_ref_anchor_writer();
        auto ref_anchor_id = ref_anchor_writer.insert_row();
        EXPECT_NE(ref_anchor_id, c_invalid_gaia_id);
    }
    auto gaia_ref_anchor_id = gaia_ref_anchor_t::insert_row();
    EXPECT_NE(gaia_ref_anchor_id, c_invalid_gaia_id);

    commit_transaction();
}

TEST_F(gaia_core_tables_test, gaia_database_gaia_table)
{
    begin_transaction();
    // gaia_database
    auto parent_id = gaia_database_t::insert_row(c_sample_name, "");
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = c_sample_name;
    auto child_id = table_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_database_t::get(parent_id);
    parent.gaia_tables().insert(child_id);
    for (auto& it : parent.gaia_tables())
    {
        EXPECT_STREQ(it.name(), c_sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_core_tables_test, gaia_table_gaia_field)
{
    begin_transaction();
    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = c_sample_name;
    auto parent_id = table_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_field
    auto field_writer = gaia_field_writer();
    field_writer.name = c_sample_name;
    auto child_id = field_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_table_t::get(parent_id);
    parent.gaia_fields().insert(child_id);
    for (auto& it : parent.gaia_fields())
    {
        EXPECT_STREQ(it.name(), c_sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_core_tables_test, gaia_table_gaia_relationship_parent)
{
    begin_transaction();
    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = c_sample_name;
    auto parent_id = table_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_relationship
    auto relationship_writer = gaia_relationship_writer();
    relationship_writer.name = c_sample_name;
    auto child_id = relationship_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_table_t::get(parent_id);
    parent.outgoing_relationships().insert(child_id);
    for (auto& it : parent.outgoing_relationships())
    {
        EXPECT_STREQ(it.name(), c_sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_core_tables_test, gaia_table_gaia_relationship_child)
{
    begin_transaction();
    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = c_sample_name;
    auto parent_id = table_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_relationship
    auto relationship_writer = gaia_relationship_writer();
    relationship_writer.name = c_sample_name;
    auto child_id = relationship_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_table_t::get(parent_id);
    parent.incoming_relationships().insert(child_id);
    for (auto& it : parent.incoming_relationships())
    {
        EXPECT_STREQ(it.name(), c_sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_core_tables_test, DISABLED_gaia_ruleset_gaia_rule)
{
    begin_transaction();
    // gaia_ruleset
    auto parent_id = gaia_ruleset_t::insert_row(c_sample_name, "");
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_rule
    auto child_id = gaia_rule_t::insert_row(c_sample_name);
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_ruleset_t::get(parent_id);
    parent.gaia_rules().insert(child_id);
    for (auto& it : parent.gaia_rules())
    {
        EXPECT_STREQ(it.name(), c_sample_name);
    }
    commit_transaction();
}

TEST_F(gaia_core_tables_test, DISABLED_gaia_table_gaia_index)
{
    begin_transaction();
    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = c_sample_name;
    auto parent_id = table_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // gaia_index
    auto index_writer = gaia_index_writer();
    index_writer.name = c_sample_name;
    auto child_id = index_writer.insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_table_t::get(parent_id);
    parent.gaia_indexes().insert(child_id);
    for (auto& it : parent.gaia_indexes())
    {
        EXPECT_STREQ(it.name(), c_sample_name);
    }
    commit_transaction();
}
