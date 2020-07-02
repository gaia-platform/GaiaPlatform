/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <list>

#include <synchronization.hpp>

namespace gaia
{
namespace db
{
namespace storage
{

static const uint64_t c_invalid_locator = 0;

class locator_allocator_t
{
protected:

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    locator_allocator_t(const locator_allocator_t&) = delete;
    locator_allocator_t& operator=(const locator_allocator_t&) = delete;

    // locator_allocator_t is a singleton, so its constructor is not public.
    locator_allocator_t();

public:

    // Return a pointer to the singleton instance.
    static locator_allocator_t* get();

    // Get a locator value.
    uint64_t allocate_locator();

    // Release a previously obtained locator value.
    void release_locator(uint64_t locator);

protected:

    // The singleton instance.
    static locator_allocator_t s_locator_allocator;

    // List of previously allocated locators that have been released
    // and are available for reuse.
    std::list<uint64_t> m_available_locators;

    // The next available locator value from our global pool.
    uint64_t m_next_locator;
};

}
}
}
