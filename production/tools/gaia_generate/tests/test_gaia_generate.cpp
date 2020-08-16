/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include "gtest/gtest.h"
#include "gaia_catalog.hpp"
#include "gaia_catalog_internal.hpp"
#include "gaia_parser.hpp"
#include "db_test_base.hpp"

using namespace gaia::catalog;
using namespace gaia::db;
using namespace std;

class gaia_generate_test : public db_test_base_t {
};

// Using the catalog manager's create_table(), create a catalog and an EDC header from that.
TEST_F(gaia_generate_test, use_create_table) {
    create_database("airport");
    ddl::field_def_list_t fields;
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"name", data_type_t::e_string, 1}));
    create_table("airport", "airport", fields);

    auto header_str = gaia_generate("airport");
    EXPECT_NE(0, header_str.find("struct airport_t"));
}

// Start from Gaia DDL to create an EDC header.
TEST_F(gaia_generate_test, parse_ddl) {
    parser_t parser;

    EXPECT_EQ(EXIT_SUCCESS, parser.parse_line("create table tmp_airport ( name string );"));
    create_database("tmp_airport");
    execute("tmp_airport", parser.statements);

    auto header_str = gaia_generate("tmp_airport");
    EXPECT_NE(0, header_str.find("struct tmp_airport_t"));
}
