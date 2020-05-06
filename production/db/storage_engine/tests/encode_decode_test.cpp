
#include "rdb_object_converter.hpp"
#include "gtest/gtest.h"
#include "storage_engine.hpp"

TEST(encode_decode_test, node) {
    // Encode node and decode node
    struct gaia::db::gaia_se_node node;
    node.id = 1;
    node.type = 1;
    node.payload[0] = 'h';
    node.payload[1] = 'i';
    node.payload_size = 2 * sizeof(node.payload[0]);

    gaia::db::string_writer key; 
    gaia::db::string_writer value;
    gaia::db::rdb_object_converter_util::encode_node(&node, &key, &value);

    gaia::db::gaia_id_t id;
    gaia::db::gaia_type_t type;
    u_int32_t size;
    const char* payload = gaia::db::rdb_object_converter_util::decode_object(key.to_slice(), value.to_slice(), false, &id, &type, &size, nullptr, nullptr);  

    EXPECT_EQ(id, 1);
    EXPECT_EQ(type, 1);
    EXPECT_EQ(size, node.payload_size);
    EXPECT_STREQ(payload, "hi");
}
