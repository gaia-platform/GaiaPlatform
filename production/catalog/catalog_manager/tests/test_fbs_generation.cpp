/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_catalog.hpp"
#include "flatbuffers/idl.h"
#include "gtest/gtest.h"

using namespace gaia::catalog;

TEST(fbs_generation_test, generate_fbs) {
    gaia::db::gaia_mem_base::init(true);
    string test_table_name{"test_fbs_generation"};

    ddl::field_definition_t f1{"id", ddl::data_type_t::INT8, 1};
    ddl::field_definition_t f2{"name", ddl::data_type_t::STRING, 1};
    vector<ddl::field_definition_t *> fields{&f1, &f2};

    create_table(test_table_name, fields);
    string fbs = generate_fbs();

    flatbuffers::Parser fbs_parser;

    EXPECT_GT(fbs.size(), 0);
    ASSERT_TRUE(fbs_parser.Parse(fbs.c_str()));
}
