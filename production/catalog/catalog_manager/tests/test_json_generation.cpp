/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "flatbuffers/idl.h"
#include "gtest/gtest.h"

#include "db_test_base.hpp"
#include "fbs_generator.hpp"
#include "gaia_catalog.hpp"
#include "json_generator.hpp"

using namespace gaia::catalog;
using namespace gaia::catalog::ddl;

class json_generation_test : public db_test_base_t
{
protected:
    static void SetUpTestSuite()
    {
        test_table_fields.emplace_back(make_unique<field_definition_t>("id", data_type_t::e_int8, 1));
        test_table_fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        test_table_fields.emplace_back(make_unique<field_definition_t>("value", data_type_t::e_float, 1));
        test_table_fields.emplace_back(make_unique<field_definition_t>("is_active", data_type_t::e_bool, 1));
        test_table_fields.emplace_back(make_unique<field_definition_t>("phone_numbers", data_type_t::e_string, 0));
        test_table_fields.emplace_back(make_unique<field_definition_t>("last_zipcodes", data_type_t::e_int32, 0));
    }

    void SetUp() override
    {
        db_test_base_t::SetUp();
    }

    static field_def_list_t test_table_fields;
};

field_def_list_t json_generation_test::test_table_fields{};

constexpr char c_expected_json[] // NOLINT
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

TEST_F(json_generation_test, generate_json_from_catalog)
{
    string test_table_name{"test_generate_json_from_catalog"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);

    string fbs = generate_fbs(table_id);
    cout << "fbs:\n" << fbs << endl;
    string json = generate_json(table_id);
    cout << "json:\n" << json << endl;

    validate_through_flatbuffers_parser(fbs, json);
}

TEST_F(json_generation_test, generate_json_from_table_definition)
{
    string test_table_name{"test_generate_json_from_table_definition"};

    string fbs = generate_fbs("", test_table_name, test_table_fields);
    string json = generate_json("", test_table_name, test_table_fields);

    validate_through_flatbuffers_parser(fbs, json);
}

TEST_F(json_generation_test, generate_bin)
{
    string test_table_name{"test_generate_bin"};

    string fbs = generate_fbs("", test_table_name, test_table_fields);
    string json = generate_json("", test_table_name, test_table_fields);
    string bin = generate_bin(fbs, json);

    // The generated bin is base64 encoded.
    // There is not much validation we can do for the base64 beforing decoding.
    // The get_bfbs test below will validate the result using FlatBuffers reflection APIs.
    ASSERT_GT(bin.size(), 0);
}

TEST_F(json_generation_test, get_bin)
{
    string test_table_name{"test_get_bin"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);

    begin_transaction();
    string bfbs = get_bfbs(table_id);
    string bin = get_bin(table_id);
    commit_transaction();

    const uint8_t* binary_schema = reinterpret_cast<const uint8_t*>(bfbs.c_str());
    const uint8_t* serialized_data = reinterpret_cast<const uint8_t*>(bin.c_str());
    size_t serialized_data_size = bin.size();

    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    ASSERT_TRUE(schema != nullptr);

    // Get the type of the schema's root object.
    const reflection::Object* root_type = schema->root_table();
    ASSERT_TRUE(root_type != nullptr);

    ASSERT_TRUE(flatbuffers::Verify(*schema, *root_type, serialized_data, serialized_data_size));
}
