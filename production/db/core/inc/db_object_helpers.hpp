/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>

#include <map>

#include "gaia/common.hpp"

#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "db_hash_map.hpp"
#include "db_internal_types.hpp"
#include "memory_types.hpp"
#include "persistence_types.hpp"

namespace gaia
{
namespace db
{

// Helper methods to properly construct and allocate an object given either
// references + data or the whole payload.

inline db_object_t* create_object(
    gaia::common::gaia_id_t id, gaia::common::gaia_type_t type,
    size_t num_refs, const gaia::common::gaia_id_t* refs,
    size_t obj_data_size, const void* obj_data)
{
    size_t ref_len = num_refs * sizeof(*refs);
    size_t total_len = obj_data_size + ref_len;
    gaia::db::hash_node_t* hash_node = db_hash_map::insert(id);
    hash_node->locator = allocate_locator();
    allocate_object(hash_node->locator, total_len + c_db_object_header_size);
    db_object_t* obj_ptr = locator_to_ptr(hash_node->locator);
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->num_references = num_refs;
    obj_ptr->payload_size = total_len;
    if (num_refs > 0)
    {
        memcpy(obj_ptr->payload, reinterpret_cast<const uint8_t*>(refs), ref_len);
    }
    memcpy(obj_ptr->payload + ref_len, obj_data, obj_data_size);
    return obj_ptr;
}

inline db_object_t* create_object(
    gaia::common::gaia_id_t id, gaia::common::gaia_type_t type,
    size_t num_refs, size_t obj_data_size,
    const void* obj_data)
{
    gaia::db::hash_node_t* hash_node = db_hash_map::insert(id);
    hash_node->locator = allocate_locator();
    gaia::db::allocate_object(hash_node->locator, obj_data_size + c_db_object_header_size);
    db_object_t* obj_ptr = locator_to_ptr(hash_node->locator);
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->num_references = num_refs;
    obj_ptr->payload_size = obj_data_size;
    memcpy(obj_ptr->payload, obj_data, obj_data_size);
    return obj_ptr;
}

inline db_object_t* id_to_ptr(common::gaia_id_t id)
{
    gaia_locator_t locator = gaia::db::db_hash_map::find(id);
    ASSERT_INVARIANT(
        locator_exists(locator),
        "An invalid locator was returned by db_hash_map::find()!");
    return locator_to_ptr(locator);
}

inline void populate_unique_offsets(
    read_record_t* record,
    std::map<common::gaia_id_t, uint8_t*>* id_to_offset,
    bool is_recovery)
{
    ASSERT_PRECONDITION(record->header.record_type == record_type_t::txn, "Expected transaction record.");
    auto payload_ptr = reinterpret_cast<uint8_t*>(record->payload);
    auto end = reinterpret_cast<uint8_t*>(record) + record->header.payload_size - (sizeof(common::gaia_id_t) * record->header.count);

    // Iterate records and find offsets to unique objects.
    while (payload_ptr < end)
    {
        auto obj_ptr = reinterpret_cast<db_object_t*>(payload_ptr);

        ASSERT_INVARIANT(obj_ptr, "Object cannot be null.");
        ASSERT_INVARIANT(obj_ptr->id != common::c_invalid_gaia_id, "Recovered id cannot be invalid.");

        size_t allocation_size = ((obj_ptr->payload_size + 2 * sizeof(db_object_t) + gaia::db::memory_manager::c_slot_size - 1) / gaia::db::memory_manager::c_slot_size) * gaia::db::memory_manager::c_slot_size;

        ASSERT_INVARIANT(allocation_size > 0 && allocation_size % gaia::db::memory_manager::c_slot_size == 0, "Invalid allocation size.");

        if (is_recovery)
        {
            auto locator = gaia::db::db_hash_map::find(obj_ptr->id);
            if (locator_exists(locator))
            {
                // Move onto next gaia object.
                payload_ptr += allocation_size;
                continue;
            }
        }

        if (id_to_offset->count(obj_ptr->id) == 0)
        {
            // Offset doesn't exist.
            id_to_offset->insert(std::pair(obj_ptr->id, reinterpret_cast<uint8_t*>(payload_ptr)));
        }
        else
        {
            // Offset exists. Update it.
            auto itr = id_to_offset->find(obj_ptr->id);
            itr->second = reinterpret_cast<uint8_t*>(payload_ptr);
        }

        payload_ptr += allocation_size;
    }
}

} // namespace db
} // namespace gaia
