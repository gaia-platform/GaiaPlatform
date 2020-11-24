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
#include "memory_manager.hpp"
#include "memory_types.hpp"
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
    data* data = gaia::db::get_shared_data();
    gaia_id_t id = __sync_add_and_fetch(&data->last_id, 1);
    return id;
}

inline gaia_type_t allocate_type()
{
    data* data = gaia::db::get_shared_data();
    gaia_type_t type = __sync_add_and_fetch(&data->last_type_id, 1);
    return type;
}

inline gaia_txn_id_t allocate_txn_id()
{
    data* data = gaia::db::get_shared_data();
    gaia_txn_id_t txn_id = __sync_add_and_fetch(&data->last_txn_id, 1);
    return txn_id;
}

inline gaia_locator_t allocate_locator()
{
    locators* locators = gaia::db::get_shared_locators();
    data* data = gaia::db::get_shared_data();

    if (!locators)
    {
        throw transaction_not_open();
    }

    // We need an acquire barrier before reading `last_locator`. We can
    // change this full barrier to an acquire barrier when we change to proper
    // C++ atomic types.
    __sync_synchronize();

    if (data->last_locator >= c_max_locators)
    {
        throw oom();
    }

    return __sync_add_and_fetch(&data->last_locator, 1);
}

inline gaia_offset_t get_gaia_offset(memory_manager::address_offset_t offset)
{
    return offset / sizeof(uint64_t);
}

inline void allocate_object(
    gaia_locator_t locator,
    memory_manager::address_offset_t offset)
{
    locators* locators = gaia::db::get_shared_locators();
    if (!locators)
    {
        throw transaction_not_open();
    }

    (*locators)[locator] = get_gaia_offset(offset);
}

inline bool locator_exists(gaia_locator_t locator)
{
    locators* locators = gaia::db::get_shared_locators();
    data* data = gaia::db::get_shared_data();

    // We need an acquire barrier before reading `last_locator`. We can
    // change this full barrier to an acquire barrier when we change to proper
    // C++ atomic types.
    __sync_synchronize();

    return (locator != c_invalid_gaia_locator)
        && (locator <= data->last_locator)
        && ((*locators)[locator] != c_invalid_gaia_offset);
}

inline gaia_offset_t locator_to_offset(gaia_locator_t locator)
{
    locators* locators = gaia::db::get_shared_locators();
    return locator_exists(locator)
        ? (*locators)[locator]
        : c_invalid_gaia_offset;
}

inline se_object_t* offset_to_ptr(gaia_offset_t offset)
{
    data* data = gaia::db::get_shared_data();
    return (offset != c_invalid_gaia_offset)
        ? reinterpret_cast<se_object_t*>(data->objects + offset)
        : nullptr;
}

inline se_object_t* locator_to_ptr(gaia_locator_t locator)
{
    return offset_to_ptr(locator_to_offset(locator));
}

} // namespace db
} // namespace gaia
