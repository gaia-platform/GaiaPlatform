/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>

#include "gaia/common.hpp"

#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "db_hash_map.hpp"
#include "db_helpers.hpp"
#include "db_internal_types.hpp"

namespace gaia
{
namespace db
{

// Helper methods to properly construct and allocate an object given either
// references + data or the whole payload.

inline db_object_t* create_object(
    gaia::common::gaia_id_t id, gaia::common::gaia_type_t type,
    size_t refs_count, const gaia::common::gaia_id_t* refs,
    size_t obj_data_size, const void* obj_data)
{
    size_t ref_len = refs_count * sizeof(*refs);
    size_t total_len = obj_data_size + ref_len;
    gaia_locator_t locator = allocate_locator(type);
    gaia::db::hash_node_t* hash_node = db_hash_map::insert(id);
    hash_node->locator = locator;
    gaia::db::allocate_object(locator, total_len);
    db_object_t* obj_ptr = locator_to_ptr(locator);
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->references_count = refs_count;
    obj_ptr->payload_size = total_len;
    if (refs_count > 0)
    {
        memcpy(obj_ptr->payload, reinterpret_cast<const uint8_t*>(refs), ref_len);
    }
    if (obj_data_size > 0)
    {
        memcpy(obj_ptr->payload + ref_len, obj_data, obj_data_size);
    }
    return obj_ptr;
}

inline db_object_t* create_object(
    gaia::common::gaia_id_t id, gaia::common::gaia_type_t type,
    size_t refs_count, size_t obj_data_size,
    const void* obj_data)
{
    gaia_locator_t locator = allocate_locator(type);
    gaia::db::hash_node_t* hash_node = db_hash_map::insert(id);
    hash_node->locator = locator;
    gaia::db::allocate_object(locator, obj_data_size);
    db_object_t* obj_ptr = locator_to_ptr(locator);
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->references_count = refs_count;
    obj_ptr->payload_size = obj_data_size;
    if (obj_data_size > 0)
    {
        memcpy(obj_ptr->payload, obj_data, obj_data_size);
    }
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

} // namespace db
} // namespace gaia
