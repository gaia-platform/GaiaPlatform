
#include "gtest/gtest.h"

#include "rdb_object_converter.hpp"
#include "storage_engine.hpp"

TEST(write_read_assert, basic)
{
    gaia::db::string_writer_t writer;

    u_int64_t e1 = 3423942943286;
    int64_t e2 = 4673438;
    u_int32_t e3 = 3423942946;
    std::string payload = "hello there 37462382038%%%^#*^@93)!@(*)@7  #@^!#)!@*#&";
    u_int32_t e5 = 467845;
    u_char e6 = u_char(0);
    u_char e7 = '*';
    std::string empty_payload = "";

    rocksdb::Slice s;

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
    s = writer.to_slice();
    gaia::db::string_reader_t reader(&s);

    u_int64_t v1;
    u_int32_t v2;
    u_char c;

    EXPECT_NO_THROW(reader.read_uint64(&v1));
    EXPECT_EQ(e1, v1);
    EXPECT_NO_THROW(reader.read_uint64(&v1));
    EXPECT_EQ(e2, v1);
    EXPECT_NO_THROW(reader.read_uint32(&v2));
    EXPECT_EQ(e3, v2);
    const char* c1 = reader.read(payload.length());
    EXPECT_STREQ(payload.data(), c1);
    EXPECT_NO_THROW(reader.read_uint32(&v2));
    EXPECT_EQ(e5, v2);
    EXPECT_NO_THROW(reader.read_byte(&c));
    EXPECT_EQ(e6, c);
    EXPECT_NO_THROW(reader.read_byte(&c));
    EXPECT_EQ(e7, c);
    const char* c2 = reader.read(0);
    EXPECT_STREQ(empty_payload.data(), c2);
    EXPECT_EQ(0, reader.get_remaining_len_in_bytes());
}

// This test illustrates why payload should always be stored at the end
TEST(write_read_assert, payload_edge_case)
{
    // Our payloads are at the end of the value slice
    // hence we don't need to deal with encoding empty strings.
    gaia::db::string_writer_t writer;
    u_char e = '*';
    std::string empty_payload = "";

    writer.write(empty_payload.data(), empty_payload.length());
    writer.write_uint8(e);

    rocksdb::Slice s = writer.to_slice();
    gaia::db::string_reader_t reader(&s);

    const char* c = reader.read(0);
    EXPECT_STRNE(empty_payload.data(), c);

    EXPECT_STRNE("*", c);

    u_char c2;
    reader.read_byte(&c2);
    EXPECT_EQ(e, c2);
}
