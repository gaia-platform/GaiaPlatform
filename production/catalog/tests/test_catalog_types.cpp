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
constexpr uint8_t sample_type = 0;
constexpr bool sample_boolean = true;

class gaia_catalog_types_test : public db_catalog_test_base_t
{
protected:
    gaia_catalog_types_test()
        : db_catalog_test_base_t(){};
};

// New types include gaia_table_t, gaia_application_t, gaia_ruleset_t, .... Insert one of each.
TEST_F(gaia_catalog_types_test, create_each_type)
{
    begin_transaction();
    auto database_id = gaia_database_t::insert_row("my_first_database", "");
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
    auto ruleset_id = gaia_ruleset_t::insert_row("my_first_ruleset", "serial_stream");
    EXPECT_NE(ruleset_id, c_invalid_gaia_id);
    auto rule_id = gaia_rule_t::insert_row("my_first_rule");
    EXPECT_NE(rule_id, c_invalid_gaia_id);
    auto app_id = gaia_application_t::insert_row("my_first_app");
    EXPECT_NE(app_id, c_invalid_gaia_id);
    auto app_database_id = app_database_t::insert_row();
    EXPECT_NE(app_database_id, c_invalid_gaia_id);
    auto app_ruleset_id = app_ruleset_t::insert_row(sample_boolean);
    EXPECT_NE(app_ruleset_id, c_invalid_gaia_id);
    auto ruleset_database_id = ruleset_database_t::insert_row();
    EXPECT_NE(ruleset_database_id, c_invalid_gaia_id);
    auto rule_table_id = rule_table_t::insert_row(sample_type, sample_boolean);
    EXPECT_NE(rule_table_id, c_invalid_gaia_id);
    auto rule_field_id = rule_field_t::insert_row(sample_type, sample_boolean);
    EXPECT_NE(rule_field_id, c_invalid_gaia_id);
    auto rule_relationship_id = rule_relationship_t::insert_row(sample_type);
    EXPECT_NE(rule_relationship_id, c_invalid_gaia_id);
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 1 */ gaia_database_gaia_table)
{
    begin_transaction();
    // gaia_database
    auto parent_id = gaia_database_t::insert_row(sample_name, "");
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

TEST_F(gaia_catalog_types_test, /* 2 */ gaia_table_gaia_field)
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

TEST_F(gaia_catalog_types_test, /* 3 */ gaia_table_gaia_relationship_parent)
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

TEST_F(gaia_catalog_types_test, /* 4 */ gaia_table_gaia_relationship_child)
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

TEST_F(gaia_catalog_types_test, /* 5 */ gaia_ruleset_gaia_rule)
{
    begin_transaction();
    // gaia_ruleset
    auto parent_id = gaia_ruleset_t::insert_row(sample_name, "");
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

TEST_F(gaia_catalog_types_test, /* 6 */ gaia_table_gaia_index)
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

TEST_F(gaia_catalog_types_test, /* 7 */ gaia_rule_rule_relationship)
{
    begin_transaction();
    // gaia_rule
    auto rule_id = gaia_rule_t::insert_row(sample_name);
    EXPECT_NE(rule_id, c_invalid_gaia_id);

    // rule_relationship
    auto rule_relationship_id = rule_relationship_t::insert_row(sample_type);
    EXPECT_NE(rule_relationship_id, c_invalid_gaia_id);

    auto parent = gaia_rule_t::get(rule_id);
    parent.rule_relationships().insert(rule_relationship_id);
    for (auto& it : parent.rule_relationships())
    {
        EXPECT_EQ(it.type(), sample_type);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 8 */ gaia_database_app_database)
{
    begin_transaction();
    // gaia_database
    auto database_id = gaia_database_t::insert_row(sample_name, "");
    EXPECT_NE(database_id, c_invalid_gaia_id);

    // app_database
    auto app_database_id = app_database_t::insert_row();
    EXPECT_NE(app_database_id, c_invalid_gaia_id);

    auto parent = gaia_database_t::get(database_id);
    parent.app_databases().insert(app_database_id);
    uint32_t counter = 0;
    for ([[maybe_unused]] auto& it : parent.app_databases())
    {
        ++counter;
    }
    EXPECT_EQ(counter, 1);
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 9 */ gaia_database_ruleset_database)
{
    begin_transaction();
    // gaia_database
    auto parent_id = gaia_database_t::insert_row(sample_name, "");
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // ruleset_database
    auto child_id = ruleset_database_t::insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_database_t::get(parent_id);
    parent.ruleset_databases().insert(child_id);
    uint32_t counter = 0;
    for ([[maybe_unused]] auto& it : parent.ruleset_databases())
    {
        ++counter;
    }
    EXPECT_EQ(counter, 1);
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 10 */ gaia_application_app_database)
{
    begin_transaction();
    // gaia_application
    auto parent_id = gaia_application_t::insert_row(sample_name);
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // app_database
    auto child_id = app_database_t::insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_application_t::get(parent_id);
    parent.app_databases().insert(child_id);
    uint32_t counter = 0;
    for ([[maybe_unused]] auto& it : parent.app_databases())
    {
        ++counter;
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 11 */ gaia_application_app_ruleset)
{
    begin_transaction();
    // gaia_application
    auto parent_id = gaia_application_t::insert_row(sample_name);
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // app_ruleset
    auto child_id = app_ruleset_t::insert_row(sample_boolean);
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_application_t::get(parent_id);
    parent.app_rulesets().insert(child_id);
    for (auto& it : parent.app_rulesets())
    {
        EXPECT_EQ(it.active_on_startup(), sample_boolean);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 12 */ gaia_ruleset_ruleset_database)
{
    begin_transaction();
    // gaia_ruleset
    auto parent_id = gaia_ruleset_t::insert_row(sample_name, "");
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // ruleset_database
    auto child_id = ruleset_database_t::insert_row();
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_ruleset_t::get(parent_id);
    parent.ruleset_databases().insert(child_id);
    uint32_t counter = 0;
    for ([[maybe_unused]] auto& it : parent.ruleset_databases())
    {
        ++counter;
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 13 */ gaia_ruleset_app_ruleset)
{
    begin_transaction();
    // gaia_ruleset
    auto parent_id = gaia_ruleset_t::insert_row(sample_name, "");
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // app_ruleset
    auto child_id = app_ruleset_t::insert_row(sample_boolean);
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_ruleset_t::get(parent_id);
    parent.app_rulesets().insert(child_id);
    for (auto& it : parent.app_rulesets())
    {
        EXPECT_EQ(it.active_on_startup(), sample_boolean);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 14 */ gaia_table_rule_table)
{
    begin_transaction();
    // gaia_table
    auto table_writer = gaia_table_writer();
    table_writer.name = sample_name;
    auto parent_id = table_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // rule_table
    auto child_id = rule_table_t::insert_row(sample_type, sample_boolean);
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_table_t::get(parent_id);
    parent.rule_tables().insert(child_id);
    for (auto& it : parent.rule_tables())
    {
        EXPECT_EQ(it.type(), sample_type);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 15 */ gaia_rule_rule_table)
{
    begin_transaction();
    // gaia_rule
    auto parent_id = gaia_rule_t::insert_row(sample_name);
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // rule_table
    auto child_id = rule_table_t::insert_row(sample_type, sample_boolean);
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_rule_t::get(parent_id);
    parent.rule_tables().insert(child_id);
    for (auto& it : parent.rule_tables())
    {
        EXPECT_EQ(it.type(), sample_type);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 16 */ gaia_rule_rule_field)
{
    begin_transaction();
    // gaia_rule
    auto parent_id = gaia_rule_t::insert_row(sample_name);
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // rule_field
    auto child_id = rule_field_t::insert_row(sample_type, sample_boolean);
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_rule_t::get(parent_id);
    parent.rule_fields().insert(child_id);
    for (auto& it : parent.rule_fields())
    {
        EXPECT_EQ(it.type(), sample_type);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 17 */ gaia_field_rule_field)
{
    begin_transaction();
    // gaia_field
    auto field_writer = gaia_field_writer();
    field_writer.name = sample_name;
    auto parent_id = field_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // rule_field
    auto child_id = rule_field_t::insert_row(sample_type, sample_boolean);
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_field_t::get(parent_id);
    parent.rule_fields().insert(child_id);
    for (auto& it : parent.rule_fields())
    {
        EXPECT_EQ(it.type(), sample_type);
    }
    commit_transaction();
}

TEST_F(gaia_catalog_types_test, /* 18 */ gaia_relationship_rule_relationship)
{
    begin_transaction();
    // gaia_relationship
    auto relationship_writer = gaia_relationship_writer();
    relationship_writer.name = sample_name;
    auto parent_id = relationship_writer.insert_row();
    EXPECT_NE(parent_id, c_invalid_gaia_id);

    // rule_relationship
    auto child_id = rule_relationship_t::insert_row(sample_type);
    EXPECT_NE(child_id, c_invalid_gaia_id);

    auto parent = gaia_relationship_t::get(parent_id);
    parent.rule_relationships().insert(child_id);
    for (auto& it : parent.rule_relationships())
    {
        EXPECT_EQ(it.type(), sample_type);
    }
    commit_transaction();
}
