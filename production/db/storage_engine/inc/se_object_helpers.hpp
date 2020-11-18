/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/gaia_common.hpp"

#include "db_types.hpp"
#include "se_hash_map.hpp"
#include "se_object.hpp"
#include "se_types.hpp"

namespace gaia
{
namespace db
{

// Helper methods to properly construct and allocate an object given either
// references + data or the whole payload.

inline se_object_t* create_object(
    gaia::common::gaia_id_t id, gaia::common::gaia_type_t type,
    size_t num_refs, const gaia::common::gaia_id_t* refs,
    size_t obj_data_size, const void* obj_data)
{
    size_t ref_len = num_refs * sizeof(*refs);
    size_t total_len = obj_data_size + ref_len;
    gaia::db::hash_node* hash_node = se_hash_map::insert(id);
    hash_node->locator = allocate_locator();
    allocate_object(hash_node->locator, total_len + sizeof(se_object_t));
    se_object_t* obj_ptr = locator_to_ptr(hash_node->locator);
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

inline se_object_t* create_object(
    gaia::common::gaia_id_t id, gaia::common::gaia_type_t type,
    size_t num_refs, size_t obj_data_size,
    const void* obj_data)
{
    gaia::db::hash_node* hash_node = se_hash_map::insert(id);
    hash_node->locator = allocate_locator();
    allocate_object(hash_node->locator, obj_data_size + sizeof(se_object_t));
    se_object_t* obj_ptr = locator_to_ptr(hash_node->locator);
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->num_references = num_refs;
    obj_ptr->payload_size = obj_data_size;
    memcpy(obj_ptr->payload, obj_data, obj_data_size);
    return obj_ptr;
}

inline se_object_t* id_to_ptr(gaia_id_t id)
{
    gaia_locator_t locator = gaia::db::se_hash_map::find(id);
    retail_assert(
        locator_exists(locator),
        "An invalid locator was returned by se_hash_map::find()!");
    return locator_to_ptr(locator);
}

} // namespace db
} // namespace gaia
