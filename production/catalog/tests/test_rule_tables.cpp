/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_catalog_test_base.hpp"

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;
using namespace std;

constexpr char c_application_name[] = "application_name";
constexpr char c_ruleset_name[] = "ruleset_name";
constexpr char c_rule_name[] = "rule_name";
constexpr uint8_t c_type_value = 33;
constexpr bool c_bool_value = false;

class gaia_rule_tables_test : public db_catalog_test_base_t
{
protected:
    gaia_rule_tables_test()
        : db_catalog_test_base_t()
    {
    }
};

// Insert one row of each catalog table.
TEST_F(gaia_rule_tables_test, create_each_type)
{
    gaia::catalog::initialize_catalog();

    // Create and connect rules catalog rows.
    begin_transaction();

    // These rows will be used through the end of this test.
    auto gaia_ruleset_id = gaia_ruleset_t::insert_row(c_ruleset_name, c_empty_string);
    EXPECT_NE(gaia_ruleset_id, c_invalid_gaia_id);
    auto gaia_ruleset = gaia_ruleset_t::get(gaia_ruleset_id);

    auto gaia_rule_id = gaia_rule_t::insert_row(c_rule_name, c_ruleset_name);
    EXPECT_NE(gaia_rule_id, c_invalid_gaia_id);
    auto gaia_rule = gaia_rule_t::get(gaia_rule_id);

    auto gaia_application_id = gaia_application_t::insert_row(c_application_name);
    EXPECT_NE(gaia_application_id, c_invalid_gaia_id);
    auto gaia_application = gaia_application_t::get(gaia_application_id);

    auto app_database_id = app_database_t::insert_row(c_application_name, c_invalid_gaia_id);
    EXPECT_NE(app_database_id, c_invalid_gaia_id);
    auto app_database = app_database_t::get(app_database_id);

    auto app_ruleset_id = app_ruleset_t::insert_row(c_application_name, c_ruleset_name, c_bool_value);
    EXPECT_NE(app_ruleset_id, c_invalid_gaia_id);
    auto app_ruleset = app_ruleset_t::get(app_ruleset_id);

    auto ruleset_database_id = ruleset_database_t::insert_row(c_ruleset_name, c_invalid_gaia_id);
    EXPECT_NE(ruleset_database_id, c_invalid_gaia_id);
    auto ruleset_database = ruleset_database_t::get(ruleset_database_id);

    auto rule_table_id = rule_table_t::insert_row(c_rule_name, c_invalid_gaia_id, c_type_value, c_bool_value);
    EXPECT_NE(rule_table_id, c_invalid_gaia_id);
    auto rule_table = rule_table_t::get(rule_table_id);

    auto rule_field_id = rule_field_t::insert_row(c_rule_name, c_invalid_gaia_id, c_type_value, c_bool_value);
    EXPECT_NE(rule_field_id, c_invalid_gaia_id);
    auto rule_field = rule_field_t::get(rule_field_id);

    auto rule_relationship_id = rule_relationship_t::insert_row(c_rule_name, c_invalid_gaia_id, c_type_value);
    EXPECT_NE(rule_relationship_id, c_invalid_gaia_id);
    auto rule_relationship = rule_relationship_t::get(rule_relationship_id);

    // gaia_ruleset_t -> gaia_rule_t
    for (const auto& it : gaia_ruleset.gaia_rules())
    {
        EXPECT_STREQ(it.name(), c_rule_name);
    }

    // gaia_rule_t -> rule_relationship_t
    for (const auto& it : gaia_rule.rule_relationships())
    {
        EXPECT_EQ(it.type(), c_type_value);
    }

    // gaia_application_t -> app_database_t
    auto counter = 0;
    for (const auto& it : gaia_application.app_databases())
    {
        EXPECT_EQ(it.gaia_database_id(), c_invalid_gaia_id);
        ++counter;
    }
    EXPECT_EQ(counter, 1);

    // gaia_application_t -> app_ruleset_t
    counter = 0;
    for (const auto& it : gaia_application.app_rulesets())
    {
        EXPECT_EQ(it.active_on_startup(), c_bool_value);
        ++counter;
    }
    EXPECT_EQ(counter, 1);

    // gaia_ruleset_t -> ruleset_database_t
    counter = 0;
    for (const auto& it : gaia_ruleset.ruleset_databases())
    {
        EXPECT_EQ(it.gaia_database_id(), c_invalid_gaia_id);
        ++counter;
    }
    EXPECT_EQ(counter, 1);

    // gaia_ruleset_t -> app_ruleset_t
    counter = 0;
    for (const auto& it : gaia_ruleset.app_rulesets())
    {
        EXPECT_EQ(it.active_on_startup(), c_bool_value);
        ++counter;
    }
    EXPECT_EQ(counter, 1);

    // gaia_rule_t -> rule_table_t
    counter = 0;
    for (const auto& it : gaia_rule.rule_tables())
    {
        EXPECT_EQ(it.type(), c_type_value);
        EXPECT_EQ(it.anchor(), c_bool_value);
        ++counter;
    }
    EXPECT_EQ(counter, 1);

    // gaia_rule_t -> rule_field_t
    counter = 0;
    for (const auto& it : gaia_rule.rule_fields())
    {
        EXPECT_EQ(it.type(), c_type_value);
        EXPECT_EQ(it.active(), c_bool_value);
        ++counter;
    }
    EXPECT_EQ(counter, 1);

    commit_transaction();

    // Navigate among connected rows.
    begin_transaction();

    // From the rule_field row to the ruleset_database row.
    auto ruleset_database_it = rule_field.rule().ruleset().ruleset_databases().begin();
    ruleset_database_t ruleset_database_2 = *ruleset_database_it;
    EXPECT_TRUE(ruleset_database == ruleset_database_2);
    EXPECT_STREQ(ruleset_database_2.gaia_ruleset(), c_ruleset_name);

    // From the rule_relationship row to the app_database row.
    auto app_database_it = rule_relationship.rule().ruleset().app_rulesets().begin()->application().app_databases().begin();
    app_database_t app_database_2 = *app_database_it;
    EXPECT_TRUE(app_database == app_database_2);
    EXPECT_STREQ(app_database_2.gaia_application(), c_application_name);

    commit_transaction();

    // Connect to core tables using gaia_id fields.
    begin_transaction();

    for (const auto& it : gaia_database_t::list())
    {
        if (strcmp(it.name(), "catalog") == 0)
        {
            // Connecting the app_database and ruleset_database rows to the gaia_database row.
            auto app_database_writer = app_database.writer();
            app_database_writer.gaia_database_id = it.gaia_id();
            app_database_writer.update_row();
            auto ruleset_database_writer = ruleset_database.writer();
            ruleset_database_writer.gaia_database_id = it.gaia_id();
            ruleset_database_writer.update_row();
            break;
        }
    }

    for (const auto& it : gaia_table_t::list())
    {
        if (strcmp(it.name(), "gaia_field") == 0)
        {
            // Connecting the rule_table row to the gaia_table row.
            auto rule_table_writer = rule_table.writer();
            rule_table_writer.gaia_table_id = it.gaia_id();
            rule_table_writer.update_row();
            break;
        }
    }

    for (const auto& it : gaia_field_t::list())
    {
        if (strcmp(it.name(), "repeated_count") == 0)
        {
            // Connecting the rule_field row to the gaia_field row.
            auto rule_field_writer = rule_field.writer();
            rule_field_writer.gaia_field_id = it.gaia_id();
            rule_field_writer.update_row();
            break;
        }
    }

    for (const auto& it : gaia_relationship_t::list())
    {
        if (strcmp(it.name(), "gaia_catalog_relationship_parent") == 0)
        {
            // Connecting the rule_relationship row to the gaia_relationship row.
            auto rule_relationship_writer = rule_relationship.writer();
            rule_relationship_writer.gaia_relationship_id = it.gaia_id();
            rule_relationship_writer.update_row();
            break;
        }
    }

    // Find the core table rows from the rule rows.
    auto gaia_database = gaia_database_t::get(app_database.gaia_database_id());
    EXPECT_STREQ(gaia_database.name(), c_catalog_db_name.c_str());

    gaia_database = gaia_database_t::get(ruleset_database.gaia_database_id());
    EXPECT_STREQ(gaia_database.name(), c_catalog_db_name.c_str());

    auto gaia_table = gaia_table_t::get(rule_table.gaia_table_id());
    EXPECT_STREQ(gaia_table.name(), "gaia_field");

    auto gaia_field = gaia_field_t::get(rule_field.gaia_field_id());
    EXPECT_STREQ(gaia_field.name(), "repeated_count");

    auto gaia_relationship = gaia_relationship_t::get(rule_relationship.gaia_relationship_id());
    EXPECT_STREQ(gaia_relationship.name(), "gaia_catalog_relationship_parent");

    commit_transaction();
}
