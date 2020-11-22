/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia/db/catalog.hpp"
#include "gaia/rules/rules.hpp"
#include "db_test_base.hpp"
#include "db_test_helpers.hpp"
#include "gaia_catalog.h"
#include "rule_checker.hpp"

using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace gaia::rules;
using namespace std;

gaia_type_t g_table_type = c_invalid_gaia_type;
map<string, uint16_t> g_field_positions;

void load_catalog()
{
    // Load the catalog with the following schema:
    // table Sensors {
    //    inactive : string,
    //    active : float, // active
    //    deprecated : uint64 // deprecated
    //    valid : uint64 // active
    //}
    ddl::field_def_list_t fields;
    string name;
    gaia_id_t table_id = c_invalid_gaia_id;

    name = "Sensors";
    // not active, not deprecated
    fields.emplace_back(make_unique<ddl::data_field_def_t>("inactive", data_type_t::e_string, 1));
    // active, not deprecated
    fields.emplace_back(make_unique<ddl::data_field_def_t>("active", data_type_t::e_float, 1));
    // deprecated
    fields.emplace_back(make_unique<ddl::data_field_def_t>("deprecated", data_type_t::e_uint64, 1));
    // another valid field
    fields.emplace_back(make_unique<ddl::data_field_def_t>("valid", data_type_t::e_uint64, 1));

    // The type of the table is the row id of table in the catalog.
    table_id = create_table(name, fields);

    // Modify the fields to have the correct active and deprecated attributes.
    begin_transaction();
    g_table_type = gaia_table_t::get(table_id).type();
    auto field_ids = list_fields(table_id);
    for (gaia_id_t field_id : field_ids)
    {
        gaia_field_t field = gaia_field_t::get(field_id);
        gaia_field_writer writer = field.writer();
        g_field_positions[field.name()] = field.position();

        if (0 == strcmp(field.name(), "active") || (0 == strcmp(field.name(), "valid")))
        {
            writer.active = true;
        }
        else if (0 == strcmp(field.name(), "deprecated"))
        {
            writer.deprecated = true;
        }
        else
        {
            continue;
        }
        writer.update_row();
    }
    commit_transaction();
}

extern "C" void initialize_rules()
{
}

class rule_checker_test : public db_test_base_t
{
public:
    void verify_exception(const char* expected_message, std::function<void()> fn)
    {
        bool did_throw = false;
        try
        {
            fn();
        }
        catch (const exception& e)
        {
            // Find a relevant substring for this message
            string str = e.what();
            // Uncomment print if you need to debug message failures.
            // printf("%s\n", str.c_str());
            size_t found = str.find(expected_message);
            EXPECT_TRUE(found != string::npos);
            did_throw = true;
        }
        EXPECT_TRUE(did_throw);
    }

protected:
    // Manage the database setup and teardown ourselves.  In ctest
    // the *Suite methods will be called for every test since every
    // test case is run in a separate process.  Outside of ctest
    // these functions will only be called once for all tests.
    static void SetUpTestSuite()
    {
        reset_server();
        begin_session();
        load_catalog();
    }
    static void TearDownTestSuite()
    {
        end_session();
    }

    void SetUp() override
    {
    }
    void TearDown() override
    {
    }
};

TEST_F(rule_checker_test, table_not_found)
{
    const gaia_type_t gaia_type = 1000;
    rule_checker_t rule_checker;
    const char* message = "Table (type:1000) was not found";
    verify_exception(message, [&]() { rule_checker.check_catalog(gaia_type, empty_fields); });
}

TEST_F(rule_checker_test, table_found)
{
    rule_checker_t rule_checker;
    rule_checker.check_catalog(g_table_type, empty_fields);
}

TEST_F(rule_checker_test, field_not_found)
{
    const field_position_t field = 1000;
    rule_checker_t rule_checker;
    field_position_list_t fields;
    fields.emplace_back(field);
    const char* message = "Field (position:1000) was not found in table";
    verify_exception(message, [&]() { rule_checker.check_catalog(g_table_type, fields); });
}

TEST_F(rule_checker_test, active_field)
{
    rule_checker_t rule_checker;

    field_position_list_t fields;
    fields.emplace_back(g_field_positions["active"]);
    rule_checker.check_catalog(g_table_type, fields);
}

TEST_F(rule_checker_test, inactive_field)
{
    rule_checker_t rule_checker;
    field_position_list_t fields;
    fields.emplace_back(g_field_positions["inactive"]);
    const char* message = "not marked as active";

    verify_exception(message, [&]() { rule_checker.check_catalog(g_table_type, fields); });
}

TEST_F(rule_checker_test, deprecated_field)
{
    rule_checker_t rule_checker;
    field_position_list_t fields;
    fields.emplace_back(g_field_positions["deprecated"]);
    const char* message = "deprecated";

    verify_exception(message, [&]() { rule_checker.check_catalog(g_table_type, fields); });
}

TEST_F(rule_checker_test, multiple_valid_fields)
{
    rule_checker_t rule_checker;
    field_position_list_t fields;
    fields.emplace_back(g_field_positions["active"]);
    fields.emplace_back(g_field_positions["valid"]);
    rule_checker.check_catalog(g_table_type, fields);
}

TEST_F(rule_checker_test, multiple_invalid_fields)
{
    rule_checker_t rule_checker;
    field_position_list_t fields;
    fields.emplace_back(g_field_positions["inactive"]);
    fields.emplace_back(g_field_positions["deprecated"]);
    // Mainly checking we get any field exception here so just
    // search for the (position: substring without specifying
    // which of the two fields above failed first.
    const char* message = "(position:";

    verify_exception(message, [&]() { rule_checker.check_catalog(g_table_type, fields); });
}

TEST_F(rule_checker_test, multiple_fields)
{
    rule_checker_t rule_checker;
    field_position_list_t fields;
    fields.emplace_back(g_field_positions["active"]);
    fields.emplace_back(g_field_positions["inactive"]);
    const char* message = "not marked as active";

    verify_exception(message, [&]() { rule_checker.check_catalog(g_table_type, fields); });
}
