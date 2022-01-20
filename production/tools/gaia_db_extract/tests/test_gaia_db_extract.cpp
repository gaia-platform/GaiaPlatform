/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>
#include <json.hpp>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/db/db_test_base.hpp"

#include "gaia_db_extract.hpp"

using namespace gaia::catalog;
using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::tools::db_extract;
using namespace std;
using json_t = nlohmann::json;

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

// TODO: This test is unstable as IDs are not guaranteed to be the same.
// Compare a JSON-formatted query result to a known good result. Use a JSON
// comparison method that is white-space and ordering independent.
TEST_F(gaia_db_extract_test, extract_catalog)
{
    create_database("extract_test", false);
    create_table(c_table_name, test_table_fields);

    // The gaia_db_extract_initialize() is actually only needed if rows must be found
    // through reflection. So this should work.
    auto extracted_catalog = gaia_db_extract("", "", c_start_at_first, c_row_limit_unlimited);
    size_t field_count = 0;

    json_t json_object = json_t::parse(extracted_catalog);

    for (auto& json_databases : json_object["databases"])
    {
        if (!json_databases["name"].get<string>().compare("extract_test"))
        {
            for (const auto& json_tables : json_databases["tables"])
            {
                for (const auto& json_fields : json_tables["fields"])
                {
                    if (!json_fields["name"].get<string>().compare("larger_value"))
                    {
                        EXPECT_EQ(json_fields["type"].get<string>().compare("double"), 0);
                        field_count++;
                    }
                    else if (!json_fields["name"].get<string>().compare("value"))
                    {
                        EXPECT_EQ(json_fields["type"].get<string>().compare("float"), 0);
                        field_count++;
                    }
                    else if (!json_fields["name"].get<string>().compare("age"))
                    {
                        EXPECT_EQ(json_fields["type"].get<string>().compare("int8"), 0);
                        field_count++;
                    }
                    else if (!json_fields["name"].get<string>().compare("name"))
                    {
                        EXPECT_EQ(json_fields["type"].get<string>().compare("string"), 0);
                        field_count++;
                    }
                    else if (!json_fields["name"].get<string>().compare("id"))
                    {
                        EXPECT_EQ(json_fields["type"].get<string>().compare("int64"), 0);
                        field_count++;
                    }
                }
            }
        }
    }

    EXPECT_EQ(field_count, 5);
}

// Exercise the ability to scan through a database table as a sequence of blocks
// of table rows. Verify that rows have been scanned and that the last of the rows
// has been read.
TEST_F(gaia_db_extract_test, extract_catalog_rows)
{
    create_database("extract_test", false);
    create_table(c_table_name, test_table_fields);
    create_relationship(
        "parent",
        {"", c_table_name, "children", "", c_table_name, relationship_cardinality_t::many},
        {"", c_table_name, "parent", "", c_table_name, relationship_cardinality_t::one},
        false);

    // Initialization is needed when using reflection.
    ASSERT_TRUE(gaia_db_extract_initialize());

    // Fetch one row at a time, from the beginning.
    uint64_t row_id = c_start_at_first;
    for (;;)
    {
        auto extracted_rows = gaia_db_extract("catalog", "gaia_field", row_id, 3);
        if (!extracted_rows.compare(c_empty_object))
        {
            break;
        }
        json_t json_object = json_t::parse(extracted_rows);
        for (auto& json_rows : json_object["rows"])
        {
            // This obtains the last seen row_id.
            row_id = json_rows["row_id"].get<uint64_t>();
        }
    }
    ASSERT_NE(row_id, c_start_at_first);
}
