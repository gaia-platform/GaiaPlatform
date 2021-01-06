/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/exception.hpp"
#include "db_types.hpp"
#include "gaia_db_internal.hpp"
#include "retail_assert.hpp"
#include "se_object.hpp"
#include "se_shared_data.hpp"
#include "se_types.hpp"

namespace gaia
{
namespace db
{

constexpr size_t c_page_size_bytes = 4096ULL;

inline size_t get_page_index_from_offset(gaia_offset_t offset)
{
    // Convert the word-size offset into bytes.
    size_t byte_offset = offset * sizeof(uint64_t);
    // Calculate the index of the page in the data segment.
    return byte_offset / c_page_size_bytes;
}

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

inline void allocate_object(gaia_locator_t locator, size_t size)
{
    std::cerr << "In allocate_object()" << std::endl;
    locators_t* locators = gaia::db::get_shared_locators();
    shared_data_t* data = gaia::db::get_shared_data();
    page_alloc_counts_t* page_alloc_counts = gaia::db::get_shared_page_alloc_counts();
    if (data->objects[0] >= std::size(data->objects))
    {
        throw oom();
    }
    // We use the first 64-bit word in the data array for the next available
    // offset, so we need to return the end of the previous allocation as the
    // current allocation's offset and add 1 to account for the first word.
    gaia_offset_t offset = 1 + __sync_fetch_and_add(&data->objects[0], (size + sizeof(int64_t) - 1) / sizeof(int64_t));
    // We need to increment the refcount for any pages touched by this object.
    // First calculate the page index.
    size_t page_index = get_page_index_from_offset(offset);
    // Increment the allocation count for the page containing this object.
    size_t old_count = (*page_alloc_counts)[page_index].fetch_add(1);
    std::cerr << "Incrementing allocation count for page index " << page_index << ", old value was " << old_count << std::endl;
    // Update the object's locator entry with this offset.
    (*locators)[locator] = offset;
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
        ? (*locators)[locator]
        : c_invalid_gaia_offset;
}

inline se_object_t* offset_to_ptr(gaia_offset_t offset)
{
    shared_data_t* data = gaia::db::get_shared_data();
    return (offset != c_invalid_gaia_offset)
        ? reinterpret_cast<se_object_t*>(data->objects + offset)
        : nullptr;
}

inline se_object_t* locator_to_ptr(gaia_locator_t locator)
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
