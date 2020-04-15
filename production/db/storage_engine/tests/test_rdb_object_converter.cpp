
#include "rdb_object_converter.hpp"
#include "gtest/gtest.h"
#include "storage_engine.hpp"

TEST(write_read_assert, basic) {
    gaia::db::string_writer writer;

    u_int64_t e1 = 3423942943286;
    u_int64_t e2 = 4673438;
    u_int32_t e3 = 3423942946;
    std::string payload = "hello there 37462382038%%%^#*^@93)!@(*)@7  #@^!#)!@*#&";
    u_int32_t e5 = 467845;
    u_char e6 = u_char (0);
    u_char e7 = '*';
    std::string empty_payload = "";

    rocksdb::Slice s;

    // Fill write buffer
    writer.write_uint64(e1);
    ASSERT_EQ(8, writer.get_current_pos());
    writer.write_uint64(e2);
    ASSERT_EQ(16, writer.get_current_pos());
    writer.write_uint32(e3);
    writer.write(payload.c_str(), payload.length());
    writer.write_uint32(e5);
    writer.write_byte(e6);
    writer.write_byte(e7);

    writer.write(empty_payload.c_str(), empty_payload.length());

    // Create reader & read in the same order of writes
    s = writer.to_slice();
    gaia::db::string_reader reader(&s);

    u_int64_t v1;
    u_int32_t v2;
    u_char c;

    ASSERT_TRUE(reader.read_uint64(&v1));
    ASSERT_EQ(e1, v1);
    ASSERT_TRUE(reader.read_uint64(&v1));
    ASSERT_EQ(e2, v1);
    ASSERT_TRUE(reader.read_uint32(&v2));
    ASSERT_EQ(e3, v2);
    const char* c1 = reader.read(payload.length());
    ASSERT_STREQ(payload.data(), c1);
    ASSERT_TRUE(reader.read_uint32(&v2));
    ASSERT_EQ(e5, v2);
    ASSERT_TRUE(reader.read_byte(&c));
    ASSERT_EQ(e6, c);
    ASSERT_TRUE(reader.read_byte(&c));
    ASSERT_EQ(e7, c);
    const char* c2 = reader.read(0);
    ASSERT_STREQ(empty_payload.data(), c2);
    ASSERT_EQ(0, reader.get_remaining_len_in_bytes());
}

// This test illustrates why payload should always be stored at the end 
TEST(write_read_assert, payload_edge_case) {
    // Our payloads are at the end of the value slice 
    // hence we don't need to deal with encoding empty strings.
    gaia::db::string_writer writer;
    u_char e = '*';
    std::string empty_payload = "";

    writer.write(empty_payload.data(), empty_payload.length());
    writer.write_byte(e);

    rocksdb::Slice s = writer.to_slice();
    gaia::db::string_reader reader(&s);

    const char* c = reader.read(0);
    ASSERT_STRNE(empty_payload.data(), c);

    ASSERT_STRNE("*", c);
    
    u_char c2;
    reader.read_byte(&c2);
    ASSERT_EQ(e, c2);     
}

// Test to encode, decode & assert contents of node obj
TEST(encode_decode_obj, node) {
    gaia::db::rdb_object_converter_util converter;
    std::string p = "hello";
    uint64_t x = 1;

    gaia::db::string_writer k;
    gaia::db::string_writer v;
    k.clean();
    v.clean();

    converter.encode_node(1, 1, p.length(), p.c_str(), &k, &v);

    rocksdb::Slice s1 = k.to_slice();
    rocksdb::Slice s2 = v.to_slice();

    u_int64_t id;
    u_int64_t type;
    u_int32_t size;
    const char* res = converter.decode_node(s1, s2, &id, &type, &size);

    ASSERT_EQ(x, type);
    ASSERT_EQ(x, id);
    ASSERT_EQ(p.length(), size);
    ASSERT_STREQ(p.c_str(), res);
}

TEST(encode_decode_obj, edge) {
    gaia::db::rdb_object_converter_util converter;
    std::string p = "hello";
    uint64_t x = 1;

    gaia::db::string_writer k;
    gaia::db::string_writer v;
    k.clean();
    v.clean();

    converter.encode_edge(x, x, p.length(), p.c_str(), x, x, &k, &v);

    rocksdb::Slice s1 = k.to_slice();
    rocksdb::Slice s2 = v.to_slice();

    u_int64_t id;
    u_int64_t type;
    u_int32_t size;
    u_int64_t first;
    u_int64_t second;
    const char* res = converter.decode_edge(s1, s2, &id, &type, &size, &first, &second);

    ASSERT_EQ(x, type);
    ASSERT_EQ(x, id);
    ASSERT_EQ(x, first);
    ASSERT_EQ(x, second);
    ASSERT_EQ(p.length(), size);
    ASSERT_STREQ(p.c_str(), res);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
