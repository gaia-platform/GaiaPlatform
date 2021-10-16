/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/db/db_test_base.hpp"

#include "gaia_airport.h"
#include "gaia_db_extract.hpp"

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;
using namespace std;

class gaia_db_extract_test : public db_test_base_t
{
};

constexpr gaia_id_t c_start = 1;
constexpr gaia_id_t c_end = 2;

TEST_F(gaia_db_extract_test, dump)
{
    create_database("airport_test");
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1));
    create_table("airport_test", "airport", fields);

    int line_limit = -1;
    vector<gaia_id_t> type_vec;
    auto dump_str = gaia_db_extract("", "", 0, 0);
    EXPECT_NE(0, dump_str.find("references=01"));
    EXPECT_NE(0, dump_str.find("references=04"));
}
