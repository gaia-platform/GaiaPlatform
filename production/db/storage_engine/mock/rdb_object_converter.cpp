/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rdb_object_converter.hpp"

using namespace gaia::common;
using namespace gaia::db;

/**
 * Format:
 * Key: fbb_type, id (uint64, uint64)
 * Value: value_type, payload_size, payload
 */
void rdb_object_converter_util::encode_node(const u_int64_t id,
                                            u_int64_t type,
                                            u_int32_t size,
                                            const char* payload,
                                            string_writer* key,
                                            string_writer* value) {
    // create key
    key->write_uint64(type);
    key->write_uint64(id);

    // create value
    value->write_uint8(GaiaObjectType::node);
    value->write_uint32(size);
    value->write(payload, size);
}

/**
 * Todo: Update to create and return gaia_ptr<node>, pending recovery impl.
 */
const char* rdb_object_converter_util::decode_node(const rocksdb::Slice& key,
                                                   const rocksdb::Slice& value,
                                                   gaia_id_t* id,
                                                   gaia_type_t* type,
                                                   u_int32_t* size) {
    string_reader key_(&key);
    string_reader value_(&value);
    //Read key.
    key_.read_uint64(type);
    key_.read_uint64(id);
    assert(key_.get_remaining_len_in_bytes() == 0);

    //Read value.
    u_char type_;
    value_.read_byte(&type_);
    assert(type_ == GaiaObjectType::node);

    value_.read_uint32(size);
    return value_.read(*size);
}

/**
 * Return whether the slice value belongs to an edge.
 */
bool rdb_object_converter_util::is_rdb_object_edge(const rocksdb::Slice& value) {
    assert(value.data());
    const u_char* type = reinterpret_cast<const u_char*>(value.data());
    return *type == GaiaObjectType::edge;
}

/**
 * Format:
 * Key: fbb_type, id (uint64, uint64)
 * Value: value_type, node_first, node_second, payload_size, payload
 */
void rdb_object_converter_util::encode_edge(const u_int64_t id,
                                            u_int64_t type,
                                            u_int32_t size,
                                            const char* payload,
                                            const u_int64_t first,
                                            const u_int64_t second,
                                            string_writer* key,
                                            string_writer* value) {

    // Create key.
    key->write_uint64(type);
    key->write_uint64(id);

    // Create value.
    value->write_uint8(GaiaObjectType::edge);
    value->write_uint64(first);
    value->write_uint64(second);
    value->write_uint32(size);
    value->write(payload, size);
}

/**
 * Todo: Update to create and return gaia_ptr<edge>, pending recovery impl.
 */
const char* rdb_object_converter_util::decode_edge(const rocksdb::Slice& key,
                                                   const rocksdb::Slice& value,
                                                   gaia_id_t* id,
                                                   gaia_type_t* type,
                                                   u_int32_t* size,
                                                   gaia_id_t* first,
                                                   gaia_id_t* second) {
    string_reader key_(&key);
    string_reader value_(&value);

    // Read key.
    key_.read_uint64(type);
    key_.read_uint64(id);

    // Read value.
    u_char type_;
    value_.read_byte(&type_);
    assert(type_ == GaiaObjectType::edge);

    value_.read_uint64(first);
    value_.read_uint64(second);

    value_.read_uint32(size);
    return value_.read(*size);
}
