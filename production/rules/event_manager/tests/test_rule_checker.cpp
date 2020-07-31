/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"
#include "rules.hpp"
#include "db_test_helpers.hpp"
#include "rule_checker.hpp"
#include "gaia_catalog.hpp"
#include "catalog_gaia_generated.h"

using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace gaia::rules;
using namespace std;


gaia_id_t g_table_type = 0;
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

    name = "Sensors";
    // not active, not deprecated
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"inactive", data_type_t::e_string, 1}));
    // active, not deprecated
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"active", data_type_t::e_float32, 1}));
    // deprecated
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"deprecated", data_type_t::e_uint64, 1}));
    // another valid field
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"valid", data_type_t::e_uint64, 1}));

    // The type of the table is the row id of table in the catalog.
    g_table_type = create_table(name, fields);

    // Modify the fields to have the correct active and deprecated attributes.
    auto field_ids = list_fields(g_table_type);
    begin_transaction();
    for (gaia_id_t field_id : field_ids)
    {
        Gaia_field field = Gaia_field::get(field_id);
        Gaia_field_writer writer = field.writer();
        g_field_positions[field.name()] = field.position();

        if (0 == strcmp(field.name(), "active") 
            || (0 == strcmp(field.name(), "valid")))
        {
            writer.active = true;
        }
        else
        if (0 == strcmp(field.name(), "deprecated"))
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

extern "C"
void initialize_rules()
{
}

class rule_checker_test : public ::testing::Test
{
public:
    void verify_exception(const char* expected_message, std::function<void ()> fn)
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
    static void SetUpTestSuite() {
        start_server();
        begin_session();
        gaia::catalog::initialize_catalog();
        load_catalog();
    }

    static void TearDownTestSuite() {
        end_session();
        stop_server();
    }

    void SetUp() override {
    }

    void TearDown() override {
    }

};

TEST_F(rule_checker_test, table_not_found)
{
    rule_checker_t rule_checker;
    const char* message = "Table (type:1000) was not found";
    verify_exception(message, [&](){
        rule_checker.check_catalog(1000, empty_fields);
    });
}

TEST_F(rule_checker_test, table_found)
{
    rule_checker_t rule_checker;
    rule_checker.check_catalog(g_table_type, empty_fields);
}

TEST_F(rule_checker_test, field_not_found)
{
    rule_checker_t rule_checker;
    field_list_t fields;
    fields.insert(1000);
    const char* message = "Field (position:1000) was not found in table";
    verify_exception(message, [&](){
        rule_checker.check_catalog(g_table_type, fields);
    });
}

TEST_F(rule_checker_test, active_field)
{
    rule_checker_t rule_checker;

    field_list_t fields;
    fields.insert(g_field_positions["active"]);
    rule_checker.check_catalog(g_table_type, fields);
}

TEST_F(rule_checker_test, inactive_field)
{
    rule_checker_t rule_checker;
    field_list_t fields;
    fields.insert(g_field_positions["inactive"]);
    const char* message = "not marked as active";

    verify_exception(message, [&](){
        rule_checker.check_catalog(g_table_type, fields);
    });
}

TEST_F(rule_checker_test, deprecated_field)
{
    rule_checker_t rule_checker;
    field_list_t fields;
    fields.insert(g_field_positions["deprecated"]);
    const char* message = "deprecated";

    verify_exception(message, [&](){
        rule_checker.check_catalog(g_table_type, fields);
    });
}

TEST_F(rule_checker_test, multiple_valid_fields)
{
    rule_checker_t rule_checker;
    field_list_t fields;
    fields.insert(g_field_positions["active"]);
    fields.insert(g_field_positions["valid"]);
    rule_checker.check_catalog(g_table_type, fields);
}

TEST_F(rule_checker_test, multiple_invalid_fields)
{
    rule_checker_t rule_checker;
    field_list_t fields;
    fields.insert(g_field_positions["inactive"]);
    fields.insert(g_field_positions["deprecated"]);
    // Mainly checking we get any field exception here so just
    // search for the (position: substring without specifying
    // which of the two fields above failed first.
    const char* message = "(position:";

    verify_exception(message, [&](){
        rule_checker.check_catalog(g_table_type, fields);
    });
}

TEST_F(rule_checker_test, multiple_fields)
{
    rule_checker_t rule_checker;
    field_list_t fields;
    fields.insert(g_field_positions["active"]);
    fields.insert(g_field_positions["inactive"]);
    const char* message = "not marked as active";

    verify_exception(message, [&](){
        rule_checker.check_catalog(g_table_type, fields);
    });
}
