/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "rdb_object_converter.hpp"

#include "db_internal_types.hpp"
#include "db_object_helpers.hpp"
#include "persistent_store_manager.hpp"
#include "record_list_manager.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::storage;

namespace gaia
{
namespace db
{

void encode_object_base(
    common::gaia_id_t id,
    common::gaia_type_t type,
    gaia::common::reference_offset_t num_references,
    uint16_t payload_size,
    const gaia::common::gaia_id_t* references,
    const char* payload,
    gaia::db::persistence::string_writer_t& key,
    gaia::db::persistence::string_writer_t& value)
{
    // Create key.
    key.write_uint64(id);

    // Create value.
    value.write_uint32(type);
    value.write_uint16(num_references);
    value.write_uint16(payload_size);

    auto reference_arr_ptr = references;
    for (int i = 0; i < num_references; i++)
    {
        // Encode all references.
        value.write_uint64(*reference_arr_ptr);
        reference_arr_ptr++;
    }

    size_t references_size = num_references * sizeof(gaia_id_t);
    size_t data_size = payload_size - references_size;
    const char* data_ptr = payload + references_size;
    value.write(data_ptr, data_size);
}

/**
 * Format:
 * Key: id (uint64)
 * Value: type, reference_count, payload_size, payload
 */
void gaia::db::persistence::encode_object(
    const db_object_t* gaia_object,
    gaia::db::persistence::string_writer_t& key,
    gaia::db::persistence::string_writer_t& value)
{
    encode_object_base(
        gaia_object->id,
        gaia_object->type,
        gaia_object->num_references,
        gaia_object->payload_size,
        gaia_object->references(),
        gaia_object->payload,
        key,
        value);
}

void gaia::db::persistence::encode_checkpointed_object(
    const db_recovered_object_t* gaia_object,
    string_writer_t& key,
    string_writer_t& value)
{
    encode_object_base(
        gaia_object->id,
        gaia_object->type,
        gaia_object->num_references,
        gaia_object->payload_size,
        gaia_object->references(),
        gaia_object->payload,
        key,
        value);
}

db_object_t* gaia::db::persistence::decode_object(
    const rocksdb::Slice& key,
    const rocksdb::Slice& value)
{
    gaia_id_t id;
    gaia_type_t type;
    uint16_t size;
    uint16_t num_references;
    gaia::db::persistence::string_reader_t key_reader(key);
    gaia::db::persistence::string_reader_t value_reader(value);

    // Read key.
    key_reader.read_uint64(id);
    ASSERT_POSTCONDITION(key_reader.get_remaining_len_in_bytes() == 0, "Detected extra data when reading key!");

    // Read value.
    value_reader.read_uint32(type);
    value_reader.read_uint16(num_references);
    value_reader.read_uint16(size);

    // Read references.
    gaia_id_t refs[num_references];
    for (size_t i = 0; i < num_references; i++)
    {
        value_reader.read_uint64(refs[i]);
    }

    auto data_size = size - num_references * sizeof(gaia_id_t);
    auto payload = value_reader.read(data_size);
    ASSERT_POSTCONDITION(payload, "Read object shouldn't be null");

    // Create object.
    db_object_t* db_object = create_object(id, type, num_references, refs, data_size, payload);

    // Lookup object again to find its locator and add it to the type's record_list.
    gaia_locator_t locator = gaia::db::db_hash_map::find(db_object->id);
    std::shared_ptr<record_list_t> record_list = record_list_manager_t::get()->get_record_list(type);
    record_list->add(locator);

    return db_object;
}

} // namespace db
} // namespace gaia
