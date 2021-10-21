/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/db/db_test_base.hpp"

#include "gaia_db_extract.hpp"

using namespace gaia::catalog;
using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::db;
using namespace std;

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

constexpr char c_expected_catalog[]
    = "CREATE FOREIGN TABLE \"test_table\"(\n"
      "gaia_id BIGINT,\n"
      "\"id\" BIGINT,\n"
      "\"name\" TEXT,\n"
      "\"age\" SMALLINT,\n"
      "\"value\" REAL,\n"
      "\"larger_value\" DOUBLE PRECISION,\n"
      "\"parent\" BIGINT\n"
      ") SERVER test_server;\n";

TEST_F(gaia_db_extract_test, extract_catalog)
{
    create_database("extract_test", false);
    gaia_id_t table_id = create_table(c_table_name, test_table_fields);
    create_relationship(
        "parent",
        {"", c_table_name, "children", "", c_table_name, relationship_cardinality_t::many},
        {"", c_table_name, "parent", "", c_table_name, relationship_cardinality_t::one},
        false);

    auto extracted_catalog = gaia_db_extract("", "", c_start_after_none, c_row_limit_unlimited);
    cout << extracted_catalog << endl;

    // ASSERT_STREQ(c_expected_catalog, extracted_catalog.c_str());
}
