/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "flatbuffers/idl.h"
#include "gtest/gtest.h"

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_test_base.hpp"

#include "fbs_generator.hpp"
#include "json_generator.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::db;

class json_generation_test : public db_test_base_t
{
protected:
    static void SetUpTestSuite()
    {
        test_table_fields.emplace_back(make_unique<data_field_def_t>("id", data_type_t::e_int8, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("value", data_type_t::e_float, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("is_active", data_type_t::e_bool, 1));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("phone_numbers", data_type_t::e_string, 0));
        test_table_fields.emplace_back(make_unique<data_field_def_t>("last_zipcodes", data_type_t::e_int32, 0));
    }

    static field_def_list_t test_table_fields;
};

field_def_list_t json_generation_test::test_table_fields;

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

TEST_F(json_generation_test, generate_json_from_catalog)
{
    string test_table_name{"test_generate_json_from_catalog"};

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);

    string fbs = generate_fbs(table_id);
    string json = generate_json(table_id);

    validate_through_flatbuffers_parser(fbs, json);
}

TEST_F(json_generation_test, generate_json_from_table_definition)
{
    string test_table_name{"test_generate_json_from_table_definition"};

    string fbs = generate_fbs("", test_table_name, test_table_fields);
    string json = generate_json(test_table_fields);

    validate_through_flatbuffers_parser(fbs, json);
}

TEST_F(json_generation_test, generate_bin)
{
    string test_table_name{"test_generate_bin"};

    string fbs = generate_fbs("", test_table_name, test_table_fields);
    string json = generate_json(test_table_fields);
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
    auto bfbs = gaia_table_t::get(table_id).binary_schema();
    vector<uint8_t> bin = get_bin(table_id);
    commit_transaction();

    const uint8_t* binary_schema = bfbs->data();
    const uint8_t* serialized_data = bin.data();
    size_t serialized_data_size = bin.size();

    const reflection::Schema* schema = reflection::GetSchema(binary_schema);
    ASSERT_TRUE(schema != nullptr);

    // Get the type of the schema's root object.
    const reflection::Object* root_type = schema->root_table();
    ASSERT_TRUE(root_type != nullptr);

    ASSERT_TRUE(flatbuffers::Verify(*schema, *root_type, serialized_data, serialized_data_size));
}
