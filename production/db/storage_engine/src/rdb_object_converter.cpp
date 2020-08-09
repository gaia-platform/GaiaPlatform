/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rdb_object_converter.hpp"
#include "gaia_ptr.hpp"

using namespace gaia::common;
using namespace gaia::db;

/**
 * Format:
 * Key: fbb_type, id (uint64, uint64)
 * Value: reference_count, payload_size, payload
 */
void rdb_object_converter_util::encode_object(
    const gaia_ptr::object* gaia_object,
    string_writer* key,
    string_writer* value) {
    // Create key.
    key->write_uint64(gaia_object->type);
    key->write_uint64(gaia_object->id);

    // Create value.
    value->write_uint32(gaia_object->num_references);
    value->write_uint32(gaia_object->payload_size);
    value->write(gaia_object->payload, gaia_object->payload_size);
}

gaia_ptr rdb_object_converter_util::decode_object(
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
    key_.read_uint64(&type);
    key_.read_uint64(&id);
    assert(key_.get_remaining_len_in_bytes() == 0);

    // Find the maximum gaia_id which was last serialized to disk.
    *max_id = std::max(*max_id, id);

    // Read value.
    value_.read_uint32(&num_references);
    value_.read_uint32(&size);
    auto payload = value_.read(size);

    // Create object & skip logging updates, since this API is called on Recovery.
    return gaia_ptr::create(id, type, num_references, size, payload, false);
}
