////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <cstring>

#include <gtest/gtest.h>

#include "rdb_object_converter.hpp"

using namespace gaia::db::persistence;

TEST(db__core__rdb_object_converter__test, basic)
{
    string_writer_t writer;

    u_int64_t e1 = 3423942943286;
    int64_t e2 = 4673438;
    u_int32_t e3 = 3423942946;
    std::string payload = "hello there 37462382038%%%^#*^@93)!@(*)@7  #@^!#)!@*#&";
    u_int32_t e5 = 467845;
    uint8_t e6 = 0;
    uint8_t e7 = '*';
    std::string empty_payload = "";
    ASSERT_EQ(empty_payload.length(), 0);

    // Fill write buffer
    writer.write_uint64(e1);
    EXPECT_EQ(8, writer.get_current_position());
    writer.write_uint64(e2);
    EXPECT_EQ(16, writer.get_current_position());
    writer.write_uint32(e3);
    writer.write(payload.c_str(), payload.length());
    writer.write_uint32(e5);
    writer.write_uint8(e6);
    writer.write_uint8(e7);

    writer.write(empty_payload.c_str(), empty_payload.length());

    // Create reader & read in the same order of writes
    rocksdb::Slice s = writer.to_slice();
    string_reader_t reader(s);

    u_int64_t v1;
    u_int32_t v2;
    uint8_t c;

    EXPECT_NO_THROW(reader.read_uint64(v1));
    EXPECT_EQ(e1, v1);
    EXPECT_NO_THROW(reader.read_uint64(v1));
    EXPECT_EQ(e2, v1);
    EXPECT_NO_THROW(reader.read_uint32(v2));
    EXPECT_EQ(e3, v2);
    const char* c1 = reader.read(payload.length());
    // We can't use EXPECT_STREQ() here, since the embedded string isn't null-terminated.
    EXPECT_EQ(0, std::memcmp(payload.data(), c1, payload.length()));
    EXPECT_NO_THROW(reader.read_uint32(v2));
    EXPECT_EQ(e5, v2);
    EXPECT_NO_THROW(reader.read_byte(c));
    EXPECT_EQ(e6, c);
    EXPECT_NO_THROW(reader.read_byte(c));
    EXPECT_EQ(e7, c);
    // We should get a null pointer from a read after we've exhausted the buffer
    // (otherwise we'd be reading past the end of the buffer).
    const char* c2 = reader.read(0);
    EXPECT_EQ(nullptr, c2);
    EXPECT_EQ(0, reader.get_remaining_len_in_bytes());
}

// This test illustrates why payload should always be stored at the end
TEST(db__core__rdb_object_converter__test, payload_edge_case)
{
    // Our payloads are at the end of the value slice
    // hence we don't need to deal with encoding empty strings.
    string_writer_t writer;
    uint8_t e = '*';
    std::string empty_payload = "";

    writer.write(empty_payload.data(), empty_payload.length());
    writer.write_uint8(e);

    rocksdb::Slice s = writer.to_slice();
    string_reader_t reader(s);

    const char* c = reader.read(0);
    // We can't use EXPECT_STRNE() here, since the embedded string isn't null-terminated.
    EXPECT_EQ(0, std::memcmp(empty_payload.data(), c, empty_payload.length()));

    uint8_t c2;
    reader.read_byte(c2);
    EXPECT_EQ(e, c2);
}
