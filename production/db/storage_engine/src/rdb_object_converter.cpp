/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rdb_object_converter.hpp"
#include "persistent_store_manager.hpp"
#include "storage_engine.hpp"

using namespace gaia::common;
using namespace gaia::db;

/**
 * Format:
 * Key: id (uint64)
 * Value: container_id, reference_count, payload_size, payload
 */
void gaia::db::encode_object(
    const gaia_se_object_t* gaia_object,
    string_writer* key,
    string_writer* value) {
    // Create key.
    key->write_uint64(gaia_object->id);

    // Create value.
    value->write_uint64(gaia_object->container_id);
    value->write_uint64(gaia_object->num_references);
    value->write_uint64(gaia_object->payload_size);

    value->write(gaia_object->payload, gaia_object->payload_size);
}

gaia_id_t gaia::db::decode_object(
    const rocksdb::Slice& key,
    const rocksdb::Slice& value) {
    gaia_id_t id;
    gaia_container_id_t container_id;
    uint64_t size;
    uint64_t num_references;
    string_reader key_(&key);
    string_reader value_(&value);

    // Read key.
    key_.read_uint64(&id);
    assert(key_.get_remaining_len_in_bytes() == 0);

    // Read value.
    value_.read_uint64(&container_id);
    value_.read_uint64(&num_references);
    value_.read_uint64(&size);
    auto payload = value_.read(size);
    // Create Object.
    persistent_store_manager::create_object_on_recovery(id, container_id, num_references, size, payload);
    return id;
}
