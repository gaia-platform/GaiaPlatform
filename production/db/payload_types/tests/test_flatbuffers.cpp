/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <fstream>

#include "gtest/gtest.h"

#include "flatbuffers/idl.h"

using namespace std;

TEST(flatbuffers, json_compilation)
{
    // Load flatbuffers schema.
    string schema;
    flatbuffers::LoadFile("test_record.fbs", true, &schema);

    // Load JSON file.
    string json;
    flatbuffers::LoadFile("test_record_data.json", true, &json);

    // Set up parser.
    flatbuffers::IDLOptions options;
    options.force_defaults = true;
    flatbuffers::Parser parser(options);

    // Parse schema.
    bool result = parser.Parse(schema.c_str());
    ASSERT_EQ(true, result);

    // We don't want to generate code for the schema.
    parser.MarkGenerated();

    // Parse JSON.
    result = parser.Parse(json.c_str());
    ASSERT_EQ(true, result);

    // Output serialization corresponding to JSON data.
    result = flatbuffers::SaveFile(
        "manual_test_record.bin",
        reinterpret_cast<char *>(parser.builder_.GetBufferPointer()),
        parser.builder_.GetSize(),
        true);
    ASSERT_EQ(true, result);

    // Load flatbuffers schema.
    string build_output;
    flatbuffers::LoadFile("test_record_data.bin", true, &build_output);

    // Load flatbuffers schema.
    string our_output;
    flatbuffers::LoadFile("manual_test_record.bin", true, &our_output);

    // Check that the outputs are identical.
    ASSERT_EQ(build_output.size(), our_output.size());
    ASSERT_EQ(0, build_output.compare(our_output));
}
