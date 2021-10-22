/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/db/db_test_base.hpp"

#include "gaia_db_extract.hpp"
#include "json.hpp"

using namespace gaia::catalog;
using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::db;
using namespace std;
using namespace nlohmann;

constexpr char c_table_name[] = "test_table";

class gaia_db_extract_test : public db_test_base_t
{
protected:
    static void SetUpTestSuite()
    {
        db_test_base_t::SetUpTestSuite();

        test_table_fields.emplace_back(make_unique<data_field_def_t>("id", data_type_t::e_int64, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("age", data_type_t::e_int8, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("value", data_type_t::e_float, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("larger_value", data_type_t::e_double, 1));
    }

    static field_def_list_t test_table_fields;
};

field_def_list_t gaia_db_extract_test::test_table_fields;

// Note: the specific values of this table may change with catalog enhancements.
// If that has an effect on this test, it may be better to verify the existence
// of known fields, like "larger_value" or "age".
constexpr char c_expected_table[] = "{"
                                    "\"fields\": ["
                                    "{"
                                    "\"id\": 64,"
                                    "\"name\": \"larger_value\","
                                    "\"position\": 4,"
                                    "\"repeated_count\": 1,"
                                    "\"type\": \"double\""
                                    "},"
                                    "{"
                                    "\"id\": 63,"
                                    "\"name\": \"value\","
                                    "\"position\": 3,"
                                    "\"repeated_count\": 1,"
                                    "\"type\": \"float\""
                                    "},"
                                    "{"
                                    "\"id\": 62,"
                                    "\"name\": \"age\","
                                    "\"position\": 2,"
                                    "\"repeated_count\": 1,"
                                    "\"type\": \"int8\""
                                    "},"
                                    "{"
                                    "\"id\": 61,"
                                    "\"name\": \"name\","
                                    "\"position\": 1,"
                                    "\"repeated_count\": 1,"
                                    "\"type\": \"string\""
                                    "},"
                                    "{"
                                    "\"id\": 60,"
                                    "\"name\": \"id\","
                                    "\"position\": 0,"
                                    "\"repeated_count\": 1,"
                                    "\"type\": \"int64\""
                                    "}"
                                    "],"
                                    "\"id\": 59,"
                                    "\"name\": \"test_table\","
                                    "\"type\": 3905506732"
                                    "}";

TEST_F(gaia_db_extract_test, extract_catalog)
{
    create_database("extract_test", false);
    create_table(c_table_name, test_table_fields);

    auto extracted_catalog = gaia_db_extract("", "", c_start_after_none, c_row_limit_unlimited);

    json j = json::parse(extracted_catalog);
    json expected_json = json::parse(c_expected_table);

    bool found_match = false;

    for (auto& json_databases : j["databases"])
    {
        if (!json_databases["name"].get<string>().compare("extract_test"))
        {
            for (auto& json_tables : json_databases["tables"])
            {
                if (expected_json == json_tables)
                {
                    found_match = true;
                    break;
                }
            }
        }
    }

    ASSERT_TRUE(found_match);
}

TEST_F(gaia_db_extract_test, extract_catalog_rows)
{
    create_database("extract_test", false);
    create_table(c_table_name, test_table_fields);
    create_relationship(
        "parent",
        {"", c_table_name, "children", "", c_table_name, relationship_cardinality_t::many},
        {"", c_table_name, "parent", "", c_table_name, relationship_cardinality_t::one},
        false);

    // Fetch one row at a time, from the beginning.
    uint64_t row_id = c_start_after_none;
    for (;;)
    {
        auto extracted_rows = gaia_db_extract("catalog", "gaia_field", row_id, 3);
        if (!extracted_rows.compare("null"))
        {
            break;
        }
        json j = json::parse(extracted_rows);
        for (auto& json_rows : j["rows"])
        {
            // This obtains the last seen row_id.
            row_id = json_rows["row_id"].get<uint64_t>();
        }
    }
    ASSERT_NE(row_id, c_start_after_none);
}
