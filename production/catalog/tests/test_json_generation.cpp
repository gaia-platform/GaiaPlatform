/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <flatbuffers/idl.h>
#include <gtest/gtest.h>

#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/db/db_test_base.hpp"

#include "fbs_generator.hpp"
#include "json_generator.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;

class catalog__json_generation__test : public db_test_base_t
{
public:
    catalog__json_generation__test()
        : db_test_base_t(true, true)
    {
    }

protected:
    static void SetUpTestSuite()
    {
        db_test_base_t::SetUpTestSuite();

        test_table_fields.emplace_back(make_unique<data_field_def_t>("id", data_type_t::e_int8, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("value", data_type_t::e_float, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("is_active", data_type_t::e_bool, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("phone_numbers", data_type_t::e_string, 0));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("last_zipcodes", data_type_t::e_int32, 0));
    }

    static field_def_list_t test_table_fields;
};

field_def_list_t catalog__json_generation__test::test_table_fields;

constexpr char c_expected_json[]
    = "{\n"
      "id:0,\n"
      "name:\"\",\n"
      "value:0.0,\n"
      "is_active:false,\n"
      "phone_numbers:[\"\"],\n"
      "last_zipcodes:[0]\n"
      "}\n";

void validate_through_flatbuffers_parser(string fbs, string json)
{
    flatbuffers::Parser fbs_parser;

    ASSERT_STREQ(c_expected_json, json.c_str());

    ASSERT_TRUE(fbs_parser.Parse(fbs.c_str()));
    ASSERT_TRUE(fbs_parser.Parse(json.c_str()));
}

TEST_F(catalog__json_generation__test, generate_json_from_catalog)
{
    string test_table_name{"test_generate_json_from_catalog"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);

    auto_transaction_t txn;
    string fbs = generate_fbs(table_id);
    string json = generate_json(table_id);

    validate_through_flatbuffers_parser(fbs, json);
}

TEST_F(catalog__json_generation__test, generate_bin)
{
    string test_table_name{"test_generate_bin"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);
    auto_transaction_t txn;
    string fbs = generate_fbs(table_id);
    const vector<uint8_t> bfbs = generate_bfbs(fbs);
    ASSERT_GT(bfbs.size(), 0);

    string json = generate_json(table_id);
    const vector<uint8_t> bin = generate_bin(fbs, json);
    ASSERT_GT(bin.size(), 0);

    const uint8_t* binary_schema = bfbs.data();
    const uint8_t* serialized_data = bin.data();
    size_t serialized_data_size = bin.size();

    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    ASSERT_TRUE(schema != nullptr);

    // Get the type of the schema's root object.
    const reflection::Object* root_type = schema->root_table();
    ASSERT_TRUE(root_type != nullptr);

    ASSERT_TRUE(flatbuffers::Verify(*schema, *root_type, serialized_data, serialized_data_size));
}

TEST_F(catalog__json_generation__test, generate_bin_default)
{
    string schema{"namespace test_defaults; table test_record { prefix:uint64; data:int64 = 15; suffix:uint64; } root_type test_record;"};

    string json_with_default_data{"{ prefix: 12302652060373662634, data: 15, suffix: 12302652060373662634 }"};
    string json_with_data{"{ prefix: 12302652060373662634, data: 31, suffix: 12302652060373662634 }"};
    string json_without_data{"{ prefix: 12302652060373662634, suffix: 12302652060373662634 }"};

    vector<uint8_t> serialization_for_default_data_present = generate_bin(schema, json_with_default_data);
    vector<uint8_t> serialization_for_data_present = generate_bin(schema, json_with_data);
    vector<uint8_t> serialization_for_data_absent = generate_bin(schema, json_without_data);

    // We enable force_defaults setting, so the default data should get serialized just as the non-default data.
    ASSERT_EQ(serialization_for_default_data_present.size(), serialization_for_data_present.size());

    // Missing data does not get serialized even with force_defaults turned on.
    ASSERT_GT(serialization_for_data_present.size(), serialization_for_data_absent.size());
}
