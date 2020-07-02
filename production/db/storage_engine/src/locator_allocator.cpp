/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <locator_allocator.hpp>

#include <retail_assert.hpp>

using namespace std;
using namespace gaia::common;
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

uint64_t locator_allocator_t::allocate_locator()
{
    return c_invalid_locator;
}

void locator_allocator_t::release_locator(uint64_t locator)
{
    retail_assert(locator != c_invalid_locator, "Invalid locator value!");
    retail_assert(locator < m_next_locator, "Attempt to release an unallocated locator value!");
}
