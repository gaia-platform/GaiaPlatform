/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "access_control.hpp"

using namespace std;

using namespace gaia::db::memory_manager;

TEST(memory_manager, access_control)
{
    cout << "sizeof(access_control_t) = " << sizeof(access_control_t) << endl;

    access_control_t access_control;
    access_control_t second_access_control;
    access_lock_type_t existing_access;

    {
        auto_access_control_t auto_access;

        ASSERT_EQ(true, auto_access.try_to_lock_access(&access_control, access_lock_type_t::remove));
        ASSERT_EQ(access_lock_type_t::remove, access_control.access_lock);
        ASSERT_EQ(1, access_control.readers_count);

        ASSERT_EQ(true, auto_access.try_to_lock_access(&access_control, access_lock_type_t::update));
        ASSERT_EQ(access_lock_type_t::update, access_control.access_lock);
        ASSERT_EQ(1, access_control.readers_count);

        auto_access_control_t second_auto_access;

        ASSERT_EQ(
            false,
            second_auto_access.try_to_lock_access(&access_control, access_lock_type_t::remove, existing_access));
        ASSERT_EQ(access_lock_type_t::update, existing_access);

        ASSERT_EQ(
            true,
            second_auto_access.try_to_lock_access(&second_access_control, access_lock_type_t::remove, existing_access));
        ASSERT_EQ(access_lock_type_t::remove, second_access_control.access_lock);
        ASSERT_EQ(1, second_access_control.readers_count);
        ASSERT_EQ(access_lock_type_t::none, existing_access);

        auto_access_control_t third_auto_access;

        third_auto_access.mark_access(&access_control);
        ASSERT_EQ(2, access_control.readers_count);

        third_auto_access.mark_access(&second_access_control);
        ASSERT_EQ(1, access_control.readers_count);
        ASSERT_EQ(2, second_access_control.readers_count);
    }

    ASSERT_EQ(access_lock_type_t::none, access_control.access_lock);
    ASSERT_EQ(0, access_control.readers_count);
    ASSERT_EQ(access_lock_type_t::none, second_access_control.access_lock);
    ASSERT_EQ(0, access_control.readers_count);

    {
        auto_access_control_t auto_access;

        auto_access.mark_access(&access_control);
        ASSERT_EQ(access_lock_type_t::none, access_control.access_lock);
        ASSERT_EQ(1, access_control.readers_count);

        ASSERT_EQ(
            true,
            auto_access.try_to_lock_access(access_lock_type_t::remove));
        ASSERT_EQ(access_lock_type_t::remove, access_control.access_lock);
        ASSERT_EQ(1, access_control.readers_count);

        auto_access.release_access();
        ASSERT_EQ(access_lock_type_t::none, access_control.access_lock);
        ASSERT_EQ(0, access_control.readers_count);

        // Re-acquire access lock so we can test releasing lock only.
        ASSERT_EQ(
            true,
            auto_access.try_to_lock_access(&access_control, access_lock_type_t::remove));
        ASSERT_EQ(access_lock_type_t::remove, access_control.access_lock);
        ASSERT_EQ(1, access_control.readers_count);

        auto_access.release_access_lock();
        ASSERT_EQ(access_lock_type_t::none, access_control.access_lock);
        ASSERT_EQ(1, access_control.readers_count);
    }

    ASSERT_EQ(access_lock_type_t::none, access_control.access_lock);
    ASSERT_EQ(0, access_control.readers_count);

    {
        auto_access_control_t auto_access;

        ASSERT_EQ(
            true,
            auto_access.try_to_lock_access(&access_control, access_lock_type_t::remove));

        auto_access_control_t second_auto_access;

        ASSERT_EQ(
            false,
            second_auto_access.try_to_lock_access(&access_control, access_lock_type_t::insert, existing_access));
        ASSERT_EQ(access_lock_type_t::remove, existing_access);

        auto_access.release_access();

        ASSERT_EQ(
            true,
            second_auto_access.try_to_lock_access(&access_control, access_lock_type_t::update, existing_access));
        ASSERT_EQ(access_lock_type_t::none, existing_access);
        ASSERT_EQ(access_lock_type_t::update, access_control.access_lock);
        ASSERT_EQ(1, access_control.readers_count);

        ASSERT_EQ(
            true,
            second_auto_access.try_to_lock_access(access_lock_type_t::update, existing_access));
        ASSERT_EQ(access_lock_type_t::update, existing_access);
        ASSERT_EQ(access_lock_type_t::update, access_control.access_lock);
        ASSERT_EQ(1, access_control.readers_count);
    }
}
