/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include <locator_allocator.hpp>

using namespace std;
using namespace gaia::db::storage;

TEST(storage, locator_allocator)
{
    uint64_t locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(1, locator);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(2, locator);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(3, locator);

    locator_allocator_t::get()->release_locator(2);
    locator_allocator_t::get()->release_locator(1);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(2, locator);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(1, locator);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(4, locator);

    locator_allocator_t::get()->release_locator(3);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(3, locator);

    locator = locator_allocator_t::get()->allocate_locator();
    ASSERT_EQ(5, locator);
}
