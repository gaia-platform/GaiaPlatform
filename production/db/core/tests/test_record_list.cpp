/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "record_list.hpp"

#include "gaia_internal/db/db_types.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::db::storage;

TEST(storage, record_list)
{
    constexpr size_t c_range_size = 3;
    constexpr gaia_locator_t c_starting_locator = 1000;
    constexpr gaia_locator_t c_count_locators = 10;
    constexpr gaia_locator_t c_new_locator = 88;

    record_list_t record_list(c_range_size);

    // Add some locator values to our list.
    for (gaia_locator_t locator = c_starting_locator; locator < c_starting_locator + c_count_locators; locator++)
    {
        record_list.add(locator);
    }

    // Start an iteration over the list.
    record_iterator_t iterator;
    bool return_value = record_list.start(iterator);
    ASSERT_EQ(true, return_value);
    ASSERT_EQ(false, iterator.at_end());

    gaia_locator_t expected_locator = c_starting_locator;
    record_range_t* current_range = iterator.current_range;

    // Iterate over the list and verify content and range changes.
    // Also delete entries with odd locator values.
    do
    {
        // Verify that ranges change as expected.
        if (expected_locator > c_starting_locator
            && (expected_locator - c_starting_locator) % c_range_size == 0)
        {
            ASSERT_TRUE(iterator.current_range != current_range);
            current_range = iterator.current_range;
        }

        ASSERT_EQ(current_range, iterator.current_range);
        ASSERT_EQ(expected_locator, record_list_t::get_record_locator(iterator));

        if (expected_locator % 2 != 0)
        {
            record_list_t::delete_record(iterator);
        }

        expected_locator++;
    } while (record_list_t::move_next(iterator));

    ASSERT_EQ(true, iterator.at_end());

    // Start a new iteration over the list.
    // Only records with even locator values should now be found.
    return_value = record_list.start(iterator);
    ASSERT_EQ(true, return_value);
    ASSERT_EQ(false, iterator.at_end());

    expected_locator = c_starting_locator;

    // Iterate over the list and verify content.
    do
    {
        ASSERT_EQ(expected_locator, record_list_t::get_record_locator(iterator));

        expected_locator += 2;
    } while (record_list_t::move_next(iterator));

    ASSERT_EQ(true, iterator.at_end());

    // Compact list.
    record_list.compact();

    // Add a new locator. It should be added in a compacted list, in between the other entries.
    record_list.add(c_new_locator);

    // Perform another iteration over the list.
    // The same records with even locator values should be found.
    return_value = record_list.start(iterator);
    ASSERT_EQ(true, return_value);
    ASSERT_EQ(false, iterator.at_end());

    expected_locator = c_starting_locator;

    // Iterate over the list and verify content.
    do
    {
        // Skip our checks if we encounter the new locator value.
        gaia_locator_t current_locator = record_list_t::get_record_locator(iterator);
        if (current_locator != c_new_locator)
        {
            ASSERT_EQ(expected_locator, current_locator);
            expected_locator += 2;
        }
    } while (record_list_t::move_next(iterator));

    ASSERT_EQ(true, iterator.at_end());
}
