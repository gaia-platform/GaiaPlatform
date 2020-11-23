/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "locator_allocator.hpp"

#include "retail_assert.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::storage;

locator_allocator_t locator_allocator_t::s_locator_allocator;

locator_allocator_t* locator_allocator_t::get()
{
    return &s_locator_allocator;
}

locator_allocator_t::locator_allocator_t()
{
    m_next_locator = 1;
}

gaia_locator_t locator_allocator_t::allocate_locator()
{
    gaia_locator_t locator = c_invalid_locator;

    // First look for an available previously allocated locator.
    m_available_locators.dequeue(locator);
    if (locator != c_invalid_locator)
    {
        return locator;
    }

    // If we did not find an available previously allocated locator,
    // then we'll need to allocate a new locator.
    locator = __sync_fetch_and_add(&m_next_locator, 1);

    retail_assert(locator != c_invalid_locator, "Invalid locator value!");

    return locator;
}

void locator_allocator_t::release_locator(gaia_locator_t locator)
{
    retail_assert(locator != c_invalid_locator, "Invalid locator value!");
    retail_assert(locator < m_next_locator, "Attempt to release an unallocated locator value!");

    m_available_locators.enqueue(locator);
}
