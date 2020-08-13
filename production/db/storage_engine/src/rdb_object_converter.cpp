/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rdb_object_converter.hpp"
#include "gaia_ptr_server.hpp"
#include "storage_engine.hpp"

using namespace gaia::common;
using namespace gaia::db;

/**
 * Format:
 * Key: id (uint64)
 * Value: type, reference_count, payload_size, payload
 */
void rdb_object_converter_util::encode_object(
    const object* gaia_object,
    string_writer* key,
    string_writer* value) {
    // Create key.
    key->write_uint64(gaia_object->id);

    // Create value.
    value->write_uint64(gaia_object->type);
    value->write_uint32(gaia_object->num_references);
    value->write_uint32(gaia_object->payload_size);
    value->write(gaia_object->payload, gaia_object->payload_size);
}

gaia_ptr_server rdb_object_converter_util::decode_object(
    const rocksdb::Slice& key,
    const rocksdb::Slice& value,
    uint64_t* max_id) {
    gaia_id_t id;
    gaia_type_t type;
    uint32_t size;
    uint32_t num_references;
    string_reader key_(&key);
    string_reader value_(&value);

    // Read key.
    key_.read_uint64(&id);
    assert(key_.get_remaining_len_in_bytes() == 0);

    // Find the maximum gaia_id which was last serialized to disk.
    *max_id = std::max(*max_id, id);

    // Read value.
    value_.read_uint64(&type);
    value_.read_uint32(&num_references);
    value_.read_uint32(&size);
    auto payload = value_.read(size);

    return gaia_ptr_server::create(id, type, num_references, size, payload);
}
