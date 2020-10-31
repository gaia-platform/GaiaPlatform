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
 * Value: type, reference_count, payload_size, payload
 */
void gaia::db::encode_object(
    const gaia_se_object_t* gaia_object,
    string_writer_t* key,
    string_writer_t* value)
{
    // Create key.
    key->write_uint64(gaia_object->id);

    // Create value.
    value->write_uint32(gaia_object->type);
    value->write_uint16(gaia_object->num_references);
    value->write_uint16(gaia_object->payload_size);

    value->write(gaia_object->payload, gaia_object->payload_size);
}

void gaia::db::decode_object(
    const rocksdb::Slice& key,
    const rocksdb::Slice& value, 
    gaia_id_t* id,
    gaia_type_t* type)
{
    uint16_t size;
    uint16_t num_references;
    string_reader_t key_reader(&key);
    string_reader_t value_reader(&value);

    // Read key.
    key_reader.read_uint64(id);
    retail_assert(key_reader.get_remaining_len_in_bytes() == 0);

    // Read value.
    value_reader.read_uint32(type);
    value_reader.read_uint16(&num_references);
    value_reader.read_uint16(&size);
    auto payload = value_reader.read(size);
    // Create Object.
    persistent_store_manager::create_object_on_recovery(*id, *type, num_references, size, payload);
}
