/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rdb_object_converter.hpp"
#include "storage_engine.hpp"

using namespace gaia::db;

gaia_ptr<gaia_se_node> rdb_object_converter_util::decode_node(const rocksdb::Slice& key, const rocksdb::Slice& value) {
    gaia_id_t id;
    gaia_type_t type;
    u_int32_t size;

    const char* payload = decode_object (
        key,
        value,
        false,
        &id,
        &type,
        &size,
        nullptr,
        nullptr
    );

    return gaia_se_node::create(id, type, size, payload, false);
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
void rdb_object_converter_util::encode_edge(const gaia_se_edge* edge, string_writer* key, string_writer* value) {
    if (!edge) {
        return;
    }

    return encode_object (
        edge->id,
        edge->type,
        edge->payload_size,
        edge->payload,
        true,
        &edge->first,
        &edge->second,
        key,
        value
    );
}

/**
 * Format:
 * Key: fbb_type, id (uint64, uint64)
 * Value: value_type, payload_size, payload
 */
void rdb_object_converter_util::encode_node(const gaia_se_node* node, string_writer* key, string_writer* value) {
    if (!node) {
        return;
    }

    return rdb_object_converter_util::encode_object (
        node->id,
        node->type,
        node->payload_size,
        node->payload,
        false,
        nullptr,
        nullptr,
        key,
        value
    );
}

void rdb_object_converter_util::encode_object (
    gaia_id_t id,
    gaia_type_t type,
    u_int32_t size,
    const char* payload,
    bool is_edge,
    const gaia_id_t* first,
    const gaia_id_t* second,
    string_writer* key,
    string_writer* value
) 
{
      // Create key.
    key->write_uint64(type);
    key->write_uint64(id);

    // Create value.
    if (is_edge) {
        value->write_uint8(GaiaObjectType::edge);
        assert(first);
        assert(second);
        value->write_uint64(*first);
        value->write_uint64(*second);
    } else {
        value->write_uint8(GaiaObjectType::node);
    }
    
    value->write_uint32(size);
    value->write(payload, size);
}

std::string rdb_object_converter_util::get_error_message(const rocksdb::Status& status) {
    std::stringstream ss;
    ss << "[RocksDB] Error code: " << status.code() << " , error message: " << status.ToString();
    return ss.str();
}

gaia_ptr<gaia_se_edge> rdb_object_converter_util::decode_edge(const rocksdb::Slice& key, const rocksdb::Slice& value) {
    gaia_id_t id;
    gaia_type_t type;
    u_int32_t size;
    gaia_id_t first;
    gaia_id_t second;

    const char* payload = rdb_object_converter_util::decode_object (
        key,
        value,
        true,
        &id,
        &type,
        &size,
        &first,
        &second
    );

    // Create object 
    return gaia_se_edge::create(id, type, first, second, size, payload, false);
}

const char* rdb_object_converter_util::decode_object ( 
    const rocksdb::Slice& key,
    const rocksdb::Slice& value, 
    bool is_edge,
    gaia_id_t* id,
    gaia_type_t* type,
    u_int32_t* size,
    gaia_id_t* first,
    gaia_id_t* second
)
{
    string_reader key_(&key);
    string_reader value_(&value);

    // Read key.
    key_.read_uint64(type);
    key_.read_uint64(id);
    
    // Read value.
    u_char type_;
    value_.read_byte(&type_); 

    if (is_edge) {
        assert(type_ == GaiaObjectType::edge);
        value_.read_uint64(first);
        value_.read_uint64(second);
    } else {
        assert(type_ == GaiaObjectType::node);
    }

    value_.read_uint32(size);
    return value_.read(*size);
}


