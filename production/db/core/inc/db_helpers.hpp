/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/exception.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

#include "db_internal_types.hpp"
#include "db_shared_data.hpp"
#include "memory_manager.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{

inline gaia_id_t allocate_id()
{
    shared_counters_t* counters = gaia::db::get_shared_counters();
    gaia_id_t id = __sync_add_and_fetch(&counters->last_id, 1);
    return id;
}

inline gaia_type_t allocate_type()
{
    shared_counters_t* counters = gaia::db::get_shared_counters();
    gaia_type_t type = __sync_add_and_fetch(&counters->last_type_id, 1);
    return type;
}

inline gaia_txn_id_t allocate_txn_id()
{
    shared_counters_t* counters = gaia::db::get_shared_counters();
    gaia_txn_id_t txn_id = __sync_add_and_fetch(&counters->last_txn_id, 1);
    return txn_id;
}

inline gaia_locator_t allocate_locator()
{
    shared_counters_t* counters = gaia::db::get_shared_counters();

    // We need an acquire barrier before reading `last_locator`. We can
    // change this full barrier to an acquire barrier when we change to proper
    // C++ atomic types.
    __sync_synchronize();

    if (counters->last_locator >= c_max_locators)
    {
        throw oom();
    }

    return __sync_add_and_fetch(&counters->last_locator, 1);
}

inline size_t get_gaia_alignment_unit()
{
    return sizeof(uint64_t);
}

inline gaia_offset_t get_gaia_offset(gaia::db::memory_manager::address_offset_t offset)
{
    return offset / get_gaia_alignment_unit();
}

inline gaia::db::memory_manager::address_offset_t get_address_offset(gaia_offset_t offset)
{
    return offset * get_gaia_alignment_unit();
}

inline void update_locator(
    gaia_locator_t locator,
    gaia::db::memory_manager::address_offset_t offset)
{
    locators_t* locators = gaia::db::get_shared_locators();
    if (!locators)
    {
        throw no_open_transaction();
    }

    (*locators)[locator] = get_gaia_offset(offset);
}

inline bool locator_exists(gaia_locator_t locator)
{
    locators_t* locators = gaia::db::get_shared_locators();
    shared_counters_t* counters = gaia::db::get_shared_counters();

    // We need an acquire barrier before reading `last_locator`. We can
    // change this full barrier to an acquire barrier when we change to proper
    // C++ atomic types.
    __sync_synchronize();

    return (locator != c_invalid_gaia_locator)
        && (locator <= counters->last_locator)
        && ((*locators)[locator] != c_invalid_gaia_offset);
}

inline gaia_offset_t locator_to_offset(gaia_locator_t locator)
{
    locators_t* locators = gaia::db::get_shared_locators();
    return locator_exists(locator)
        ? (*locators)[locator].load()
        : c_invalid_gaia_offset;
}

inline db_object_t* offset_to_ptr(gaia_offset_t offset)
{
    shared_data_t* data = gaia::db::get_shared_data();
    return (offset != c_invalid_gaia_offset)
        ? reinterpret_cast<db_object_t*>(data->objects + offset)
        : nullptr;
}

inline db_object_t* locator_to_ptr(gaia_locator_t locator)
{
    return offset_to_ptr(locator_to_offset(locator));
}

// This is only meant for "fuzzy snapshots" of the current last_txn_id; there
// are no memory barriers.
inline gaia_txn_id_t get_last_txn_id()
{
    shared_counters_t* counters = gaia::db::get_shared_counters();
    return counters->last_txn_id;
}

} // namespace db
} // namespace gaia
