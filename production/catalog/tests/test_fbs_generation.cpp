/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <flatbuffers/idl.h>
#include <gtest/gtest.h>

#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/db/db_test_base.hpp"

#include "fbs_generator.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;

class fbs_generation_test : public db_test_base_t
{
protected:
    static void SetUpTestSuite()
    {
        db_test_base_t::SetUpTestSuite();

        test_table_fields.emplace_back(make_unique<ddl::data_field_def_t>("id", data_type_t::e_int8, 1));
        test_table_fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1));
    }

    static ddl::field_def_list_t test_table_fields;
};

ddl::field_def_list_t fbs_generation_test::test_table_fields;

TEST_F(fbs_generation_test, generate_fbs_from_catalog)
{
    string test_table_name{"test_fbs_generation"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);

    auto_transaction_t txn;
    string fbs = generate_fbs(table_id);

    flatbuffers::Parser fbs_parser;

    EXPECT_GT(fbs.size(), 0);
    ASSERT_TRUE(fbs_parser.Parse(fbs.c_str()));
}

TEST_F(fbs_generation_test, generate_bfbs)
{
    string test_table_name{"test_fbs_generation"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);

    auto_transaction_t txn;
    string fbs = generate_fbs(table_id);
    vector<uint8_t> bfbs = generate_bfbs(fbs);

    ASSERT_GT(bfbs.size(), 0);
    flatbuffers::Verifier verifier(bfbs.data(), bfbs.size());
    EXPECT_TRUE(reflection::VerifySchemaBuffer(verifier));

    auto& schema = *reflection::GetSchema(bfbs.data());
    auto root_table = schema.root_table();
    ASSERT_STREQ(root_table->name()->c_str(), (c_gaia_namespace + "." + c_internal_suffix + "." + test_table_name).c_str());
}
