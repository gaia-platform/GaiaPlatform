/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <flatbuffers/idl.h>
#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/db/db_test_base.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::db;

constexpr char c_table_name[] = "test_table";
constexpr char c_server_name[] = "test_server";

class fdw_ddl_generation_test : public db_test_base_t
{
public:
    fdw_ddl_generation_test()
        : db_test_base_t(true, true)
    {
    }

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

field_def_list_t fdw_ddl_generation_test::test_table_fields;

constexpr char c_expected_fdw_ddl[]
    = "CREATE FOREIGN TABLE \"test_table\"(\n"
      "gaia_id BIGINT,\n"
      "\"id\" BIGINT,\n"
      "\"name\" TEXT,\n"
      "\"age\" SMALLINT,\n"
      "\"value\" REAL,\n"
      "\"larger_value\" DOUBLE PRECISION,\n"
      "\"parent\" BIGINT\n"
      ") SERVER test_server;\n";

TEST_F(fdw_ddl_generation_test, generate_fdw_ddl_from_catalog)
{
    gaia_id_t table_id = create_table(c_table_name, test_table_fields);
    create_relationship(
        "parent",
        {"", c_table_name, "children", "", c_table_name, relationship_cardinality_t::many},
        {"", c_table_name, "parent", "", c_table_name, relationship_cardinality_t::one},
        false);

    string fdw_ddl = generate_fdw_ddl(table_id, c_server_name);

    ASSERT_STREQ(c_expected_fdw_ddl, fdw_ddl.c_str());
}
