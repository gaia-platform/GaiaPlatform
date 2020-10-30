/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "flatbuffers/idl.h"
#include "gtest/gtest.h"

#include "db_test_base.hpp"
#include "fdw_ddl_generator.hpp"
#include "gaia_catalog.hpp"

using namespace gaia::catalog;
using namespace gaia::catalog::ddl;

constexpr char c_table_name[] = "test_table";
constexpr char c_server_name[] = "test_server";

class fdw_ddl_generation_test : public db_test_base_t
{
protected:
    static void SetUpTestSuite()
    {
        test_table_fields.emplace_back(make_unique<field_definition_t>("id", data_type_t::e_int64, 1));
        test_table_fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        test_table_fields.emplace_back(make_unique<field_definition_t>("age", data_type_t::e_int8, 1));
        test_table_fields.emplace_back(make_unique<field_definition_t>("value", data_type_t::e_float, 1));
        test_table_fields.emplace_back(make_unique<field_definition_t>("larger_value", data_type_t::e_double, 1));
        test_table_fields.emplace_back(make_unique<field_definition_t>("parent", data_type_t::e_references, 1, c_table_name));
    }

    static field_def_list_t test_table_fields;
};

field_def_list_t fdw_ddl_generation_test::test_table_fields;

constexpr char c_expected_fdw_ddl[]
    = "create foreign table test_table(\n"
      "id bigint,\n"
      "name text,\n"
      "age smallint,\n"
      "value real,\n"
      "larger_value double precision,\n"
      "parent bigint\n"
      ")\n"
      "server test_server\n";

TEST_F(fdw_ddl_generation_test, generate_fdw_ddl_from_catalog)
{
    gaia_id_t table_id = create_table(c_table_name, test_table_fields);

    string fdw_ddl = generate_fdw_ddl(table_id, c_server_name);

    ASSERT_STREQ(c_expected_fdw_ddl, fdw_ddl.c_str());
}

TEST_F(fdw_ddl_generation_test, generate_fdw_ddl_from_table_definition)
{
    string fdw_ddl = generate_fdw_ddl(c_table_name, test_table_fields, c_server_name);

    ASSERT_STREQ(c_expected_fdw_ddl, fdw_ddl.c_str());
}
