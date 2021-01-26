/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gaia_internal/db/db_types.hpp"
#include "gtest/gtest.h"

#include "locator_allocator.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::db::storage;

TEST(storage, locator_allocator)
{
    // Allocate a few locators: they should be allocated incrementally from 1.
    gaia::db::gaia_locator_t locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(1, locator);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(2, locator);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(3, locator);

    // Release some locators and verify that they will be allocated next,
    // in the order of their release.
    locator_allocator_t::get()->release_locator(2);
    locator_allocator_t::get()->release_locator(1);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(2, locator);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(1, locator);

    // Verify that we continue allocating sequentially after we run out of released locators.
    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(4, locator);

    // Release one more locator and verify that it gets reused
    // before we continue allocating sequentially.
    locator_allocator_t::get()->release_locator(3);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(3, locator);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(5, locator);
}
