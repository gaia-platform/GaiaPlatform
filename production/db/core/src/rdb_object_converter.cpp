////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "rdb_object_converter.hpp"

#include "db_object_helpers.hpp"

using namespace gaia::common;
using namespace gaia::db;

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
    value.write_uint16(gaia_object->references_count);
    value.write_uint16(gaia_object->payload_size);

    auto reference_arr_ptr = gaia_object->references();
    for (int i = 0; i < gaia_object->references_count; i++)
    {
        // Encode all references.
        value.write_uint64(*reference_arr_ptr);
        reference_arr_ptr++;
    }

    size_t data_size = gaia_object->data_size();
    const char* data_ptr = gaia_object->data();
    value.write(data_ptr, data_size);
}

db_object_t* gaia::db::persistence::decode_object(
    const rocksdb::Slice& key,
    const rocksdb::Slice& value)
{
    gaia_id_t id;
    gaia_type_t type;
    uint16_t size;
    reference_offset_t::value_type references_count;
    gaia::db::persistence::string_reader_t key_reader(key);
    gaia::db::persistence::string_reader_t value_reader(value);

    // Read key.
    key_reader.read_uint64(id.value_ref());
    ASSERT_POSTCONDITION(key_reader.get_remaining_len_in_bytes() == 0, "Detected extra data when reading key!");

    // Read value.
    value_reader.read_uint32(type.value_ref());
    value_reader.read_uint16(references_count);
    value_reader.read_uint16(size);

    // Read references.
    gaia_id_t refs[references_count];
    for (size_t i = 0; i < references_count; i++)
    {
        value_reader.read_uint64(refs[i].value_ref());
    }

    auto data_size = size - references_count * sizeof(gaia_id_t);
    auto payload = value_reader.read(data_size);

    // Create object.
    db_object_t* db_object = create_object(id, type, references_count, refs, data_size, payload);

    return db_object;
}

} // namespace db
} // namespace gaia
