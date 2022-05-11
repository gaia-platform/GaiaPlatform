/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <fstream>
#include <iostream>

#include <flatbuffers/idl.h>
#include <gtest/gtest.h>

using namespace std;

constexpr char c_fbs_filename[] = "test_record.fbs";
constexpr char c_json_filename[] = "test_record_data.json";
constexpr char c_programmatic_bfbs_filename[] = "programmatic_test_record.bfbs";
constexpr char c_programmatic_bin_filename[] = "programmatic_test_record_data.bin";
constexpr char c_build_bin_filename[] = "test_record_data.bin";
constexpr char c_build_bfbs_filename[] = "test_record.bfbs";

void compare_file_content(const char* filename1, const char* filename2)
{
    string content1;
    flatbuffers::LoadFile(filename1, true, &content1);

    string content2;
    flatbuffers::LoadFile(filename2, true, &content2);

    // Check that the contents are identical.
    ASSERT_EQ(content1.size(), content2.size());
    ASSERT_EQ(0, content1.compare(content2));
}

TEST(db__payload_types__flatbuffers, fbs_compilation)
{
    // Load flatbuffers schema.
    string schema;
    flatbuffers::LoadFile(c_fbs_filename, true, &schema);

    // Set up parser.
    // No custom options are needed.
    flatbuffers::Parser parser;

    // Parse schema.
    bool result = parser.Parse(schema.c_str());
    ASSERT_TRUE(result);

    // Serialize schema definition.
    parser.Serialize();

    // Output serialized schema.
    result = flatbuffers::SaveFile(
        c_programmatic_bfbs_filename,
        reinterpret_cast<char*>(parser.builder_.GetBufferPointer()),
        parser.builder_.GetSize(),
        true);
    ASSERT_TRUE(result);

    // Compare the content of the programmatically generated files
    // with that of the build generated files.
    compare_file_content(c_build_bfbs_filename, c_programmatic_bfbs_filename);
}

TEST(db__payload_types__flatbuffers, json_compilation)
{
    // Load flatbuffers schema.
    string schema;
    flatbuffers::LoadFile(c_fbs_filename, true, &schema);

    // Load JSON file.
    string json;
    flatbuffers::LoadFile(c_json_filename, true, &json);

    // Set up parser.
    // We force the serialization of default values
    // to allow scalar fields to be updated in-place.
    flatbuffers::IDLOptions options;
    options.force_defaults = true;
    flatbuffers::Parser parser(options);

    // Parse schema.
    bool result = parser.Parse(schema.c_str());
    ASSERT_EQ(true, result);

    // Parse JSON.
    result = parser.Parse(json.c_str());
    ASSERT_EQ(true, result);

    // Output serialization corresponding to JSON data.
    result = flatbuffers::SaveFile(
        c_programmatic_bin_filename,
        reinterpret_cast<char*>(parser.builder_.GetBufferPointer()),
        parser.builder_.GetSize(),
        true);
    ASSERT_EQ(true, result);

    // Compare the content of the programmatically generated files
    // with that of the build generated files.
    compare_file_content(c_build_bin_filename, c_programmatic_bin_filename);
}
