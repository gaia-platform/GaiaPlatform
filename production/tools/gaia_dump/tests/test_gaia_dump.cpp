/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "catalog.hpp"
#include "db_test_base.hpp"
#include "gaia_airport.h"
#include "gaia_dump.hpp"

using namespace gaia::catalog;
using namespace gaia::db;
using namespace std;

class gaia_dump_test : public db_test_base_t
{
};

constexpr gaia_id_t c_start = 1;
constexpr gaia_id_t c_end = 2;

TEST_F(gaia_dump_test, dump)
{
    create_database("airport_test");
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::field_definition_t>("name", data_type_t::e_string, 1));
    create_table("airport_test", "airport", fields);

    int line_limit = -1;
    auto dump_str = gaia_dump(c_start, c_end, true, true, true, line_limit);
    EXPECT_NE(0, dump_str.find("references=01"));
    EXPECT_NE(0, dump_str.find("references=04"));
}
