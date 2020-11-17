/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "db_types.hpp"
#include "gaia_common.hpp"
#include "gaia_db.hpp"
#include "gaia_db_internal.hpp"
#include "gaia_exception.hpp"
#include "retail_assert.hpp"
#include "se_object.hpp"
#include "se_shared_data.hpp"
#include "se_types.hpp"

namespace gaia
{
namespace db
{

inline gaia_id_t allocate_id()
{
    data* data = gaia::db::get_shared_data_ptr();
    gaia_id_t id = __sync_add_and_fetch(&data->next_id, 1);
    return id;
}

inline gaia_type_t allocate_type()
{
    data* data = gaia::db::get_shared_data_ptr();
    gaia_type_t type = __sync_add_and_fetch(&data->next_type_id, 1);
    return type;
}

inline gaia_txn_id_t allocate_txn_id()
{
    data* data = gaia::db::get_shared_data_ptr();
    gaia_txn_id_t txn_id = __sync_add_and_fetch(&data->next_txn_id, 1);
    return txn_id;
}

inline gaia_locator_t allocate_locator()
{
    locators* locators = gaia::db::get_shared_locators_ptr();
    data* data = gaia::db::get_shared_data_ptr();

    if (locators == nullptr)
    {
        throw transaction_not_open();
    }

    if (data->locator_count >= c_max_locators)
    {
        throw oom();
    }

    return __sync_add_and_fetch(&data->locator_count, 1);
}

inline void allocate_object(gaia_locator_t locator, size_t size)
{
    locators* locators = gaia::db::get_shared_locators_ptr();
    data* data = gaia::db::get_shared_data_ptr();
    if (locators == nullptr)
    {
        throw transaction_not_open();
    }

    if (data->objects[0] >= c_max_objects)
    {
        throw oom();
    }

    (*locators)[locator] = __sync_add_and_fetch(&data->objects[0], (size + sizeof(uint64_t) - 1) / sizeof(uint64_t));
}

inline bool locator_exists(gaia_locator_t locator)
{
    locators* locators = gaia::db::get_shared_locators_ptr();
    return (locator && (*locators)[locator]);
}

inline gaia_offset_t locator_to_offset(gaia_locator_t locator)
{
    locators* locators = gaia::db::get_shared_locators_ptr();
    return locator_exists(locator)
        ? (*locators)[locator]
        : c_invalid_gaia_offset;
}

inline se_object_t* locator_to_ptr(gaia_locator_t locator)
{
    locators* locators = gaia::db::get_shared_locators_ptr();
    data* data = gaia::db::get_shared_data_ptr();
    return locator_exists(locator)
        ? reinterpret_cast<se_object_t*>(data->objects + (*locators)[locator])
        : nullptr;
}

} // namespace db
} // namespace gaia
