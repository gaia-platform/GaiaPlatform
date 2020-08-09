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
 * Value: reference_count, payload_size, payload
 */
void rdb_object_converter_util::encode_object(
    const uint64_t id,
    uint64_t type,
    uint32_t reference_count,
    uint32_t size,
    const char* payload,
    string_writer* key,
    string_writer* value) {
    // Create key.
    key->write_uint64(type);
    key->write_uint64(id);

    // Create value.
    value->write_uint32(reference_count);
    value->write_uint32(size);
    value->write(payload, size);
}

/**
 * Todo: Update to create and return gaia_ptr<node>, pending recovery impl.
 */
const char* rdb_object_converter_util::decode_object(
    const rocksdb::Slice& key,
    const rocksdb::Slice& value,
    gaia_id_t* id,
    gaia_type_t* type,
    uint32_t* reference_count,
    uint32_t* size,
    uint64_t* max_id) {
    string_reader key_(&key);
    string_reader value_(&value);
    // Read key.
    key_.read_uint64(type);
    key_.read_uint64(id);
    assert(key_.get_remaining_len_in_bytes() == 0);

    // Find the maximum gaia_id which was last serialized to disk.
    *max_id = std::max(*max_id, id);
    
    // Read value.
    value_.read_uint32(reference_count);
    value_.read_uint32(size);
    return value_.read(*size);
}
