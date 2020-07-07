/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_catalog.hpp"
#include "fbs_generator.hpp"
#include "flatbuffers/idl.h"
#include "gtest/gtest.h"

using namespace gaia::catalog;

class fbs_generation_test : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        gaia::db::gaia_mem_base::init(true);
        // We need to use push_back to init the test fields because:
        // 1) Initializer_lists always perform copies, and unique_ptrs are not copyable.
        // 2) Without make_unique (C++ 14), using emplace_back and new can leak if the vector fails to reallocate memory.
        test_table_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("id", ddl::data_type_t::INT8, 1)));
        test_table_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("name", ddl::data_type_t::STRING, 1)));
    }

    static ddl::field_def_list_t test_table_fields;
};

ddl::field_def_list_t fbs_generation_test::test_table_fields{};

TEST_F(fbs_generation_test, generate_fbs_from_catalog) {
    string test_table_name{"test_fbs_generation"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);
    string fbs = generate_fbs(table_id);

    flatbuffers::Parser fbs_parser;

    EXPECT_GT(fbs.size(), 0);
    ASSERT_TRUE(fbs_parser.Parse(fbs.c_str()));
}

TEST_F(fbs_generation_test, generate_fbs_from_table_definition) {
    string test_table_name{"test_fbs_generation"};

    string fbs = generate_fbs(test_table_name, test_table_fields);

    flatbuffers::Parser fbs_parser;

    EXPECT_GT(fbs.size(), 0);
    ASSERT_TRUE(fbs_parser.Parse(fbs.c_str()));
}

TEST_F(fbs_generation_test, generate_bfbs) {
    string test_table_name{"test_fbs_generation"};

    string fbs = generate_fbs(test_table_name, test_table_fields);
    string bfbs = generate_bfbs(fbs);

    // The generated bfbs is basd64 encoded.
    // There is not much validation we can do for the base64 beforing decoding.
    // The get_bfbs test below will validate the result using FlatBuffers reflection APIs.
    ASSERT_GT(bfbs.size(), 0);
}

TEST_F(fbs_generation_test, get_bfbs) {
    string test_table_name{"bfbs_test"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);
    string bfbs = get_bfbs(table_id);

    auto &schema = *reflection::GetSchema(bfbs.c_str());
    auto root_table = schema.root_table();
    ASSERT_STREQ(root_table->name()->c_str(), test_table_name.c_str());
}
