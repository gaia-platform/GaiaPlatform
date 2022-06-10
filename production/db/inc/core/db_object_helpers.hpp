////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cstring>

#include "gaia/common.hpp"

#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"

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
    // register_locator_for_id() returns false if the ID was already present in
    // the map.
    if (!register_locator_for_id(id, locator))
    {
        throw duplicate_object_id_internal(id);
    }
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
    // register_locator_for_id() returns false if the ID was already present in
    // the map.
    if (!register_locator_for_id(id, locator))
    {
        throw duplicate_object_id_internal(id);
    }
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

} // namespace db
} // namespace gaia
