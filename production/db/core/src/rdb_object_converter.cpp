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
    // Create key.
    key.write_uint64(gaia_object->id);

    // Create value.
    value.write_uint32(gaia_object->type);
    value.write_uint16(gaia_object->num_references);
    value.write_uint16(gaia_object->payload_size);

    value.write(gaia_object->payload, gaia_object->payload_size);
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
    auto payload = value_reader.read(size);

    // Create object.
    db_object_t* db_object = create_object(id, type, num_references, size, payload);

    // Lookup object again to find its locator and add it to the type's record_list.
    gaia_locator_t locator = gaia::db::db_hash_map::find(db_object->id);
    std::shared_ptr<record_list_t> record_list = record_list_manager_t::get()->get_record_list(type);
    record_list->add(locator);

    return db_object;
}

} // namespace db
} // namespace gaia
