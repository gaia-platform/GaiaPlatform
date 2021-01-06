/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <list>

#include "db_types.hpp"
#include "queue.hpp"

namespace gaia
{
namespace db
{
namespace storage
{

static constexpr gaia_locator_t c_invalid_locator = 0;

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
    gaia_locator_t allocate_locator();

    // Release a previously obtained locator value.
    void release_locator(gaia_locator_t locator);

protected:
    // The singleton instance.
    static locator_allocator_t s_locator_allocator;

    // List of previously allocated locators that have been released
    // and are available for reuse.
    gaia::common::queue_t<gaia_locator_t> m_available_locators;

    // The next available locator value from our global pool.
    gaia_locator_t m_next_locator;
};

} // namespace storage
} // namespace db
} // namespace gaia
