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
    // Size in alignment units, rounded up to the nearest alignment unit.
    size_t alignment_unit_size = (size + sizeof(int64_t) - 1) / sizeof(int64_t);
    // We use the first 64-bit word in the data array for the next available
    // offset, so we need to return the end of the previous allocation as the
    // current allocation's offset and add 1 to account for the first word.
    gaia_offset_t offset = 1 + __sync_fetch_and_add(&data->objects[0], alignment_unit_size);
    std::cerr << "Allocating object with offset " << offset << " at locator " << locator << " with size " << size << std::endl;
    // Convert the word-size offset into bytes.
    size_t byte_offset = offset * sizeof(uint64_t);
    size_t end_byte_offset = byte_offset + size;
    size_t start_page_index = byte_offset / c_page_size_bytes;
    size_t end_page_index = end_byte_offset / c_page_size_bytes;
    size_t page_byte_offset = byte_offset % c_page_size_bytes;
    std::cerr << "Object has byte offset " << page_byte_offset << " on page " << start_page_index << std::endl;

    // Increment the allocation count for each page containing this object.
    for (size_t page_index = start_page_index; page_index <= end_page_index; ++page_index)
    {
        size_t old_count = (*page_alloc_counts)[page_index].fetch_add(1);
        std::cerr << "Incrementing allocation count for page index " << page_index << ", old value was " << old_count << ", allocation offset " << offset << std::endl;
        // If this is the first allocation on a page, then add an extra refcount to
        // prevent freeing the page while it is still being used for allocations.
        if (old_count == 0)
        {
            std::cerr << "Adding extra allocation count for first allocation on page index " << page_index << std::endl;
            (*page_alloc_counts)[page_index].fetch_add(1);
            // Decrement the extra refcount on the preceding page to ensure it's freed.
            if (page_index > 0)
            {
                std::cerr << "Removing extra allocation count on page index " << (page_index - 1) << std::endl;
                (*page_alloc_counts)[page_index - 1].fetch_sub(1);
            }
        }
    }
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
