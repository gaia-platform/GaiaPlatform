/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "flatbuffers/idl.h"
#include "gtest/gtest.h"

#include "db_test_base.hpp"
#include "fbs_generator.hpp"
#include "gaia_catalog.hpp"

using namespace gaia::catalog;

class fbs_generation_test : public db_test_base_t
{
protected:
    static void SetUpTestSuite()
    {
        test_table_fields.emplace_back(make_unique<ddl::field_definition_t>("id", data_type_t::e_int8, 1));
        test_table_fields.emplace_back(make_unique<ddl::field_definition_t>("name", data_type_t::e_string, 1));
    }

    static ddl::field_def_list_t test_table_fields;
};

ddl::field_def_list_t fbs_generation_test::test_table_fields{};

TEST_F(fbs_generation_test, generate_fbs_from_catalog)
{
    string test_table_name{"test_fbs_generation"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);
    string fbs = generate_fbs(table_id);

    flatbuffers::Parser fbs_parser;

    EXPECT_GT(fbs.size(), 0);
    ASSERT_TRUE(fbs_parser.Parse(fbs.c_str()));
}

TEST_F(fbs_generation_test, generate_fbs_from_table_definition)
{
    string test_table_name{"test_fbs_generation"};

    string fbs = generate_fbs("", test_table_name, test_table_fields);

    flatbuffers::Parser fbs_parser;

    EXPECT_GT(fbs.size(), 0);
    ASSERT_TRUE(fbs_parser.Parse(fbs.c_str()));
}

TEST_F(fbs_generation_test, generate_bfbs)
{
    string test_table_name{"test_fbs_generation"};

    string fbs = generate_fbs("", test_table_name, test_table_fields);
    string bfbs = generate_bfbs(fbs);

    // The generated bfbs is basd64 encoded.
    // There is not much validation we can do for the base64 beforing decoding.
    // The get_bfbs test below will validate the result using FlatBuffers reflection APIs.
    ASSERT_GT(bfbs.size(), 0);
}

TEST_F(fbs_generation_test, get_bfbs)
{
    string test_table_name{"bfbs_test"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);
    begin_transaction();
    string bfbs = get_bfbs(table_id);
    commit_transaction();

    ASSERT_GT(bfbs.size(), 0);
    flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(bfbs.c_str()), bfbs.length());
    EXPECT_TRUE(reflection::VerifySchemaBuffer(verifier));

    auto& schema = *reflection::GetSchema(bfbs.c_str());
    auto root_table = schema.root_table();
    ASSERT_STREQ(root_table->name()->c_str(), (c_gaia_namespace + "." + test_table_name).c_str());
}
