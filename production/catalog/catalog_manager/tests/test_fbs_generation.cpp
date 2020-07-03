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
    }
};


TEST_F(fbs_generation_test, generate_fbs_from_catalog) {
    string test_table_name{"test_fbs_generation"};

    ddl::field_definition_t f1{"id", ddl::data_type_t::INT8, 1};
    ddl::field_definition_t f2{"name", ddl::data_type_t::STRING, 1};
    vector<ddl::field_definition_t *> fields{&f1, &f2};

    gaia_id_t table_id = create_table(test_table_name, fields);
    string fbs = generate_fbs(table_id);

    flatbuffers::Parser fbs_parser;

    EXPECT_GT(fbs.size(), 0);
    ASSERT_TRUE(fbs_parser.Parse(fbs.c_str()));
}

TEST_F(fbs_generation_test, generate_fbs_from_table_definition) {
    string test_table_name{"test_fbs_generation"};

    ddl::field_definition_t f1{"id", ddl::data_type_t::INT8, 1};
    ddl::field_definition_t f2{"name", ddl::data_type_t::STRING, 1};
    vector<ddl::field_definition_t *> fields{&f1, &f2};

    string fbs = generate_fbs(test_table_name, fields);

    flatbuffers::Parser fbs_parser;

    EXPECT_GT(fbs.size(), 0);
    ASSERT_TRUE(fbs_parser.Parse(fbs.c_str()));
}

TEST_F(fbs_generation_test, generate_bfbs) {
    string test_table_name{"test_fbs_generation"};

    ddl::field_definition_t f1{"id", ddl::data_type_t::INT8, 1};
    ddl::field_definition_t f2{"name", ddl::data_type_t::STRING, 1};
    vector<ddl::field_definition_t *> fields{&f1, &f2};

    string fbs = generate_fbs(test_table_name, fields);
    string bfbs = generate_bfbs(fbs);

    // The generated bfbs is basd64 encoded.
    // There is not much validation we can do for the base64 beforing decoding.
    // The get_bfbs test below will validate the result using FlatBuffers reflection APIs.
    ASSERT_GT(bfbs.size(), 0);
}

TEST_F(fbs_generation_test, get_bfbs) {
    string test_table_name{"bfbs_test"};
    ddl::field_definition_t f1{"id", ddl::data_type_t::INT8, 1};
    ddl::field_definition_t f2{"name", ddl::data_type_t::STRING, 1};
    vector<ddl::field_definition_t *> fields{&f1, &f2};

    gaia_id_t table_id = create_table(test_table_name, fields);
    string bfbs = get_bfbs(table_id);

    auto &schema = *reflection::GetSchema(bfbs.c_str());
    auto root_table = schema.root_table();
    ASSERT_STREQ(root_table->name()->c_str(), test_table_name.c_str());
}
